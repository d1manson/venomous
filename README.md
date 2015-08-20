 ![venomous logo](/img/logo.png)    
data analytics and simulation engine

**This project is still in very early development.**

So far the only tool that even vaguely works is the Venomous Explorer - [click here](http://d1manson.github.io/venomous/explorer/) to open the sample document.

### What is Venomous?   [wishful thinking...]
Venomous is a C++ framework that lets you build highly interactive applications focused around a complex, but essentially static, data analysis pipeline.  The programmer defines a set of input and compute nodes using C++ embedded in an XML tree. Then, using Venomous, this XML document is compiled into a data analysis engine in pure C++.  Once converted to standard C++, the custom-designed analysis engine can be integrated into a regular application, or turned into a library for Matlab/Python users. You could also transpile to JavaScript for use in modern browsers.    

The benefit of using Venomous is that it modularises each stage of the analysis pipeline, making it easy to develop and maintain large pipelines.   More than that though, Venomous is built with optimisation and non-blocking interactivity at its core: wherever you see an opportunity to optimize something in your pipeline Venomous has syntax to let you request that optimisation, and whatever hardware resources you have availabe, Venomous has syntax that lets you properly utilize them.   Behind the scenes Venomous deals with streaming/batching, canceling, memoisation, and ordering of tasks to maximise use of cpu/memory/disk/gpu/network.   In addition to the Venomous compiler, there are a number of extra tools to aid with design, debugging, optimising, and testing.  

In summary, by declaring your analysis pipeline using the Venomous XML/C++ syntax you will find that a range of benefits naturally arrise, at least in principle. And since Venomous is open source, you can help contribute any features that you feel are missing.

**Venomous is different from [other pipeline tools](https://github.com/pditommaso/awesome-pipeline)**: it does't just take a list of tasks and run them on separate cores/across a cluster; it doesn't compile a restricted library of processing "blocks" into a super optimised one-input-one-output kernel; it doesn't deal with database querying/filtering in any special ways; it doesn't come pre-loaded with connections to online data sources.  Instead, it's a bit like a modern web browser in terms of providing a platform that tries to automatically squeeze as much as possible out of the local hardware in order to create the most interactive experience posible for the end-user, exposing a scripting language/API to the developer to let them join in with this quest for extreme performance.

### Roadmap   
There is a lot of work to be done to even get a basic bare-bones example running.  The first step is to write a sample xml/c++ document for a full realworld pipeline, establishing a rough syntax for Venomous to use at the design end.  In parallel with this effort some C++ code will need to be written for the behind-the-scenes part of the engine, and the structure of the C++ output from the Venomous compiler needs to be decided.   Then the actual compiler needs to be constructed: this will be done in python, initially using a selection of very fragile regex manipulations, and possibly using libclang at a later date.    

Initially the implementation will ignore many of the optimisation hints from the XML document, and will target a single-threaded output.  However it must quickly progress to a multithreaded output that uses most of the syntactically permited optimisations.  Next, tools should be created in addition to the compiler, namely a static pipeline viewer, profiler, and testing facility.   Ultimately all the tools might be best provided as a cloud-based solution which lets you construct pipelines within a Venomous IDE, and easily do the compilation to C++.

At some point an auto-generated Matlab/Python binding should be created, and some effort should be made to get Venomous outputting to asm.js, NaCl, or other web technology. 

### Motivating example
The need for a framework like Venomous was identified during development of the [waveform project](https://github.com/d1manson/waveform/).  At a gross level, that project's pipeline looks roughly as follows:

![dummy_example](/img/dummy_example.png) 

The full design document for the Venomous implementation of Waveform can be viewed using the Venomous Explorer tool [here](http://d1manson.github.io/venomous/explorer/).  Note that it consits of a web of over 50 nodes, with more than one "chain" and a variety of topological motifs.


### Venomous tutorial 

When developing with Venomous you have **two completely separate roles**:
1. You are the designer of an API: using the Venomous C++/XML syntax you define both the "public" interface and the innards of your custom analysis/simulation engine.  You then give your design to the Venmous compiler, which converts it into pure C++ code.
2. You are the consumer of the API: using the pure C++ code provide by the above process you write an application which lets an end-user interact with that API through some kind of user interface.  

All the benefits of Venomous arrise as a result of this logical division, so it is critical that you understand it before going forward.

**In the designer role**, you create a directed-(nearly)acyclcic-graph, which consists of three types of nodes:
1. "input" nodes. Examples: file handle, smoothing kernel shape, region of interest.
2. "compute" nodes. Examples: node that reads a file off disk and parses its contents, node that creates a histogram from some raw data, node that finds a circle in an image.
3. "chain" nodes. This is more complciated - it's why the graph is only "nearly" asyclic. We'll come back to this later.

A stupidly **simple example of a design** using one input and one compute might look like this:
```
TODO: show planet -> planetary_greeting
```
Here the `planet` is an input node, which must be a `string` and `planetary_greeting` is a very simple compute node contain the following code: 
```
planetary_greeting = "hello " + planet;
```
Note that this example is *stupidly simple*: a real-life compute node wll normally take at least 1000 cycles to do its job and may take as long as several miliseconds or even seconds.

**In the API-consumer role**, you can have two main interactions with Venomous:
  1. You create new instances of input nodes, assigning data to them at the point of creation.
  2. You set callbacks to listen on the results of compute nodes.

**Continuing the `planetary_greeting` example above**, but now in the API-conumer role, lets say you want to display your planteary greeting at the top of the window. To do so, you must register a callback on the `planetary_greeting` compute node, and as part of this registration action you must specify which `planet` you want to be the input to your `planetary_greeting`.  There are three ways to specify this:
  1. You can assign an *immutable instance* of the `planet` node. You create such instances using real data, such as the string `"Earth"`,
  2. You can assign a *variable instance* of the `planet` node. Such instances are used for holding immutable instances - here you are free to swap the current immutable instance with another one as an when needed.
  3. You can assign a "pointer instance* of the `planet` node. Such instances point to variable instances, using the current immutable instance within the pointed-to  variable instance.  In simple applications these instances are usually not needed.
 
Note that only input (and chain) nodes can be instanteated in the above three ways - compute nodes are either private (i.e. not exposed at all by the API) or are only accessible in terms of being objects to register callbacks on.

Consider **a slightly more interesting example** in which there are now two inputs.
```
TODO: show (raw_img , filter) -> filtered_img 
```
Now in our applciation we may want to have several `raw_img`s, but only one global filter setting.  To set this up, we would create an immutable `raw_img` instance for each of our various images, and we would create a variable `filter` instance (which we would then populate with some starting immutable `filter` instance).  Then for each slot that needs to display an image we would register a callback on `filtered_img`, providing a specific immutable `raw_img` as one input and the variable `filter` as the other input.  Thus, whenever we change the filter all the callbacks will be triggered.

Note that in real-life examples there will probably be several stages of compute before you reach the "public" output compute node.  The important thing to remember is that when requesting a callback you have to **specify the complete set of input nodes** for the given compute node, i.e. the intermediary compute nodes are irrelevant, all that matters is the top-level inputs for the given output compute.  We will discuss how this works for chain nodes in a moment.  Also, note that when registering a callback, for each of the inputs we can independandlt choose to use either an immutable, a variable, or a pointer.




