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



