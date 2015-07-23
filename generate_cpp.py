# -*- coding: utf-8 -*-

#%%

fn = "sample.xml"
fn_out = "sample.cpp"
from lxml import etree as ET
from collections import OrderedDict
import re
import warnings
tree = ET.parse(fn)
indent = "    "


def strip_common_indent(s):
    s = s.split('\n')
    i = min(len(ss) - len(ss.lstrip()) for ss in s if ss.strip())
    return '\n'.join(ss[i:] for ss in s)
def add_indent(s,n=1):
    extra= ''.join([indent]*n)
    return '\n'.join(extra + ss for ss in s.splitlines())
    
hint_re = re.compile(r"(\w+)\s*=\s*(\w+)")
alias_re = re.compile(r"(\w+)\s*=\s*(\w+)")
return_re = re.compile(r"return\[\s*(\w+)\s*\]\s*=\s*([^;]+)")
ltgt_re = re.compile(r"\^\s*(\w+)\s*\^")
return_read_re = re.compile(r"return\[\s*(\w+)\s*\]")
return_computed_re = re.compile(r"computed\(\s*return\[\s*(\w+)\s*\]\s*\)")
d_input = OrderedDict()
d_compute = OrderedDict()
d_alias = OrderedDict()
d_type = OrderedDict()
raw_strs = []

def get_x(name):
    try:
        return d_compute[name]
    except KeyError:
        pass
    try:
        return d_input[name]
    except KeyError:
        pass
    try:
        return d_alias[name]
    except KeyError:
        raise Exception("could not find input/compute '" + name + "'")
    
class Input(object):
    def __init__(self, name, type_):
        self.name = name
        type_ = re.sub(ltgt_re, r"<\1>",  type_)
        self.type_ = type_
        d_type[self.name] = "struct {name}_t{{\n{type_} 0_;\n}}".format(name=name,type_=type_)
    def translated_type(self):
        return self.type_
        
class Compute(object):
    """
    There are 3 vriation on output:
            multiout = False, with type_="actual_type"
            multiout = True, with type_=["tuple","of","types"]
            multiout = True, with varout="final_type" and type_=["possibly", "empty", "tuple"]
            
    hints are stored as a dict with the obvious name,values, note that values
    are strings though.
    """
    def __init__(self,name, code, type_, takes, description="", hints=""):
        self.name = name        
        self.description = strip_common_indent(description) if description else ""
        self.code = strip_common_indent(code) if code else ""
        self.takes = tuple(x for x in takes.replace(","," ").split(" ") if x)
        self.is_nullable, self.takes = zip(*((x.endswith('?'), x.rstrip('?'))
                                              for x in self.takes))
        
        type_ = re.sub(ltgt_re, r"<\1>", type_.replace(","," "))
        self.varout = False
        if ' ' not in type_:
            self.multiout = False
            self.type_ = type_
            d_type[self.name] = "struct %s{\n%s 0_;\n}" % (self.name + "_t", self.type_)
        else:
            self.multiout = True
            self.type_ = tuple(x for x in type_.split(" ") if x)
            if self.type_[-1] == "...":
                self.varout = self.type_[-2]
                self.type_ = self.type_[:-2]
            d_type[self.name] = '\n'.join(t + " _" + str(i) + ";" for i,t in enumerate(self.type_))
            if self.varout:
                d_type[self.name] += "\nvector<" + self.varout + "> vararg;"
            d_type[self.name] = "struct %s{\n%s\n}" % (self.name + "_t", d_type[self.name])
            
        if hints:
            self.hints = {k: v for k, v in re.findall(hint_re,hints)}
        else:
            self.hints = {}  
    def stripped_code(self):
        s = self.code
        if not s:
            return ""
        s = strip_common_indent(s.strip())
        return s
        
    def translated_code(self):
        s = self.stripped_code()
        for t in self.takes:
            s = s.replace(t, t+"()")
        s = re.sub(return_re, r'sink(x_return(\1, \2))', s)
        s = re.sub(return_computed_re,  r'x_is_computed_self(\1)', s)
        s = re.sub(return_read_re, r'X_READ_SELF(\1)', s)
        return s
        
    def __repr__(self):
        return strip_common_indent("""
    class {name}_func {{
    private:
    {takes}
    
        template <typename T>
        yield_signal x_return(int n, T val){{
            // TODO: store val
            return yield_signal(WRITTEN, n);
        }}
        
        bool x_is_computed_self(int n){{
            return true;// TODO: this        
        }}
        
    public:
        void operator()(sink_t& sink){{
    {comment}
    {code}
        }}
    }}""").format(name=self.name,
                 comment=add_indent("/*\n" + self.description.strip() +
                                     "\n\n*************************************\n" +
                                    self.stripped_code() +
                                    "\n*************************************\n*/", n=2),
                 code= add_indent(self.translated_code(),n=2),
                 takes= '\n'.join(indent + "{name}_t {name}(){{\n{indent}{indent}return something;//TODO: this\n{indent}}}".format(name=t,indent=indent) 
                                  for t in self.takes))
        
    
        
def do_alias(node):
    """
    This doesn't actully recurse - I thought it might be neccessary but then
    I realised that perhaps you would never actually want to define annoynmous
    nested aliases in the way I'd first thought..but you can extent it if need be.
    """
    suffix = "_"
    node_src = node.attrib.get('src', node.tag)
    node_alias = node.attrib.get('name', node.attrib.get('withalias'))
    node_parsed = get_x(node_src)
    
    if isinstance(node_parsed, Compute):
        mapping = {k: v for k, v in re.findall(alias_re, node.text)}
        takes = [mapping.get(k,k) for k in list(node_parsed.takes)]
    else:
        takes = [node_src]        
    node_type = node_alias + suffix + "t"
    d_type[node_alias] = "struct %s{\n%s\n}" % (node_type, '\n'.join(
                            '%s _%d;' %(v + "_t",i) for i,v in enumerate(takes)))
    return node_alias
    
    
##########################################    
##########################################
    
parent = tree.getroot()
for child in parent:
    tag, name = child.tag.lower(), child.attrib.get('name',None)
    if tag == "input":
        d_input[name] = Input(name,child.attrib['type'])
    elif tag == "compute":
        hints = description = code = None
        for sub_node in child:
            if sub_node.tag.lower() == "description":
                description = sub_node.text
            elif sub_node.tag.lower() in ("hints", "hint"):
                hints = sub_node.text
            elif sub_node.tag.lower() == "code":
                code = sub_node.text
        if child.text.strip():
            warnings.warn("[Compute:" + name + "] ignoring text: " + child.text.strip() )
        d_compute[name] = Compute(name, sub_node.text, child.attrib['type'],
                                  child.attrib['takes'], description, hints)
    elif tag == "alias":
        d_alias[name] = get_x(child.attrib['src'])
        do_alias(child)
        #d_type[name] = "struct %s {\n%s value;\n}" %(name, child.attrib['src']);
    elif tag == "raw":
        raw_strs.append(strip_common_indent(child.text))
    else:
        raise Exception("what is this '{}' node?".format(tag))

##########################################
##########################################

raw_str = '\n'.join(raw_strs)
type_str = '\n\n'.join(d_type.values())
func_str = '\n\n'.join(str(x) for x in d_compute.values())

with open(fn_out,'w') as f:
    f.write(""" //generated in python from xml source
#include <string.h>
using string = std::string;
using byte = char;
enum yield_enum{
written,
required
}
struct yield_signal{
yield_enum t;
int param; // TODO: use a union and explicit typing
}
using sink_t = boost::coroutines::asymmetric_coroutine<yield_signal>::push_type;

""")
    f.write(raw_str)
    f.write(type_str)
    f.write("\n\n")
    f.write(func_str)
    