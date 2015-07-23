 ![venomous logo](/logo.png)    
data analytics and simulation engine

**This project is still in very early development.**

### What is Venomous?    
Venomous is a C++ framework that lets you build highly interactive applications focused around a complex, but essentially static, data analysis pipeline.  The programmer defines a set of input and compute nodes using C++ embedded in an XML tree. Then, using Venomous, this XML document is compiled into a data analysis engine in pure C++.  Once converted to standard C++, the custom-designed analysis engine can be integrated into a regular application, or turned into a library for Matlab/Python users. You could also transpile to JavaScript for use in modern browsers.    

The benefit of using Venomous is that it modularises each stage of the analysis pipeline, making it easy to develop and maintain large pipelines.   Venomous is also built with optimisation and non-blocking interactivity at its core: wherever you see an opportunity to optimize something in your pipeline Venomous has syntax to let you specify it.   Behind the scenes Venomous deals with streaming/batching, canceling, memoisation, and ordering of tasks to maximise use of cpu/memory/disk/gpu.  Venomous can also help you test for correctness of hand-written optimisations.   

**Example 1:** *An application simulating the temporal evolution of a system of differential equations.*    
 ![example1](/example1.png)  

**Example 2:** *An application showing separate summary plots for each of the many clusters within a dataset, where the user can modify the cluster assignment, with full redo-undo capabilities.*
 ![example2](/example2.png)  
 This is the usage case that motivated the development of Venomous.  See the [waveform project](http://www.github.com/d1manson/waveform).


### Roadmap   
There is a lot of work to be done to even get a basic bare-bones example running.  The first step is to write a sample xml/c++ document for a full realworld pipeline, establishing a rough syntax for Venomous to use at the design end.  In parallel with this effort some C++ code will need to be written for the behind-the-scenes part of the engine, and the structure of the C++ output from the Venomous compiler needs to be decided.   Then the actual compiler needs to be constructed: this will be done in python, initially using a selection of very fragile regex manipulations, and possibly using libclang at a later date.    

Initially the implementation will ignore many of the optimisation hints from the XML document, and will target a single-threaded output.  However it must quickly progress to a multithreaded output that uses most of the syntactically permited optimisations.  Next, tools should be created in addition to the compiler, namely a static pipeline viewer, profiler, and testing facility.   Ultimately all the tools might be best provided as a cloud-based solution which lets you construct pipelines within a Venomous IDE, and easily do the compilation to C++.

At some point an auto-generated Matlab/Python binding should be created, and some effort should be made to get Venomous outputting to asm.js, NaCl, or other web technology. 



