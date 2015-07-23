 //generated in python from xml source

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


struct point{
int16 x;
int16 y;
}


template<typename T>
struct spatial{
float bin_size;
int nW;
int nH;
vector<T> data; // length nW*nH
}

template<typename T>
struct slice{
T start;
T end;
}

enum shape_type{
rect;
circle;
}
struct shape{
shape_type kind;
union shape{
	struct rect{
	int W:
	int H;
	point topleft;
	}
	struct circle{
	int R;
	point centre;
	}
}
}

float boundary_dist_rect(point p, point topleft, int W, int H){
	auto v1 = p.x - topleft.x);
	auto v2 = topleft.x + W - p.x;
	auto v3 = p.y - topleft.y);
	auto v4 = topleft.y + H - p.y;
	return std::min(v1,v2,v3,v4);
};



enum logical_summary{
// recrods the stat of a bool vector: either all true, none true, or a mixture of true and false.
all; 
none;
mixture;
}



vector<int32> hist(vals, bin_size){
	vector<int32> h; // TODO: behind the scenes we need to decide how to deal with dynamically 
					 //resizing vector, which will reach a final state by the time the function returns.
	for(int i=0;i <vals.length;i++)
		h[vals[i]/bin_size]++;
	return h;
}

vector<int32> hist_masked(vals, mask, bin_size){
	vector<int32> h;
	for(int i=0;i <vals.length;i++) if(mask[i])
		h[vals[i]/bin_size]++
	return h;
}
struct axona_file_name_t{
string 0_;
}

struct axona_file_t{
map _0;
vector<byte[]> vararg;
}

struct pos_file_name_t{
axona_file_name_t _0;
}

struct pos_file_t{
pos_file_name_t _0;
}

struct set_file_name_t{
axona_file_name_t _0;
}

struct set_file_t{
set_file_name_t _0;
}

struct tet_file_name_t{
axona_file_name_t _0;
}

struct tet_file_t{
tet_file_name_t _0;
}

struct eeg_file_name_t{
axona_file_name_t _0;
}

struct eeg_file_t{
eeg_file_name_t _0;
}

struct both_xy_t{
bool _0;
point[] _1;
point[] _2;
}

struct xy_t{
point[] 0_;
}

struct dir_t{
bool _0;
float[] _1;
float[] _2;
}

struct speed_t{
int16[] 0_;
}

struct group_num_t{
uint8 0_;
}

struct trial_time_slice_t{
slice<int32> 0_;
}

struct directional_slice_t{
slice<float> 0_;
}

struct spatial_mask_t{
spatial<bool> 0_;
}

struct boundary_dist_slice_t{
slice<float> 0_;
}

struct boundary_shape_t{
shape 0_;
}

struct dist_to_boundary_t{
float[] 0_;
}

struct pos_mask_t{
logical_summary _0;
bool[] _1;
}

struct speed_bin_size_t{
float 0_;
}

struct speed_dwell_t{
int32[] 0_;
}


class axona_file_func {
private:
    axona_file_name_t axona_file_name(){
        return something;//TODO: this
    }
    
    template <typename T>
    yield_signal x_return(int n, T val){
        // TODO: store val
        return yield_signal(WRITTEN, n);
    }
    
    bool x_is_computed_self(int n){
        return true;// TODO: this        
    }

public:
    void operator()(sink_t& sink){
        /*
        Reads an axomna file which consists of a header block of multiple lines of: 
        		key_name [SPACE] key_value
        Followed by a line that begins with the characters "data_start".
        Following that is a (large) block of binary data.
        In some cases (set files) there may not be a data_start and binary_data section.
        This compute is aliased as pos_file, set_file, tet_file, and eeg_file.
        
        *************************************
        const int BLOCK_SIZE = 2*1024*104; //2MB
        FileStreamer f(axona_file_name); //this is C++ RAII in action, we are using axona_file_name from its definition as an "input".
        int n_blocks = ceil(f.length()/BLOCK_SIZE); 
        if ( !computed(return[0]) ){
        	// we may already have this cached.
        	map header;
        	// parse f into header, storing the n_block, and data_start pointer as well
        	return[0] = header; // this 0th block is of type "map", which is what we meant by "map,byte[],..."
        }
        
        for (block_ii in requested_blocks){
        	return[block_ii] = f.read(block_ii*BLOCK_SIZE, BLOCK_SIZE); // blocks 1,2,3... are of type "byte[]", which is what we mean by "byte[],..."
        }
        *************************************
        */
        const int BLOCK_SIZE = 2*1024*104; //2MB
        FileStreamer f(axona_file_name()); //this is C++ RAII in action, we are using axona_file_name() from its definition as an "input".
        int n_blocks = ceil(f.length()/BLOCK_SIZE); 
        if ( !x_is_computed_self(0) ){
        	// we may already have this cached.
        	map header;
        	// parse f into header, storing the n_block, and data_start pointer as well
        	sink(x_return(0, header)); // this 0th block is of type "map", which is what we meant by "map,byte[],..."
        }
        
        for (block_ii in requested_blocks){
        	sink(x_return(block_ii, f.read(block_ii*BLOCK_SIZE, BLOCK_SIZE))); // blocks 1,2,3... are of type "byte[]", which is what we mean by "byte[],..."
        }
    }
}


class both_xy_func {
private:
    pos_file_t pos_file(){
        return something;//TODO: this
    }
    
    template <typename T>
    yield_signal x_return(int n, T val){
        // TODO: store val
        return yield_signal(WRITTEN, n);
    }
    
    bool x_is_computed_self(int n){
        return true;// TODO: this        
    }

public:
    void operator()(sink_t& sink){
        /*
        This takes the pos header and buffer and does the pos-post-processing, to produce either 1 or two
        arrays of points (only 1 if 1 LED was used).
        
        *************************************
        return[0] = pos_file[0]['num_LEDs'] == 2;
        
        tmp_1 = array(pos_file[0]['num_samps']);
        if return[0]:
        	tmp_2 = array(pos_file[0]['num_samps']);
        
        for(int i=1, p=0; i < pos_file.length; i++){
        	byte[] block = pos_file[i];
        	for(int j=0;j < block/W;j++,p++){
        		tmp_1[p] = block[whaterver];
        		tmp_2[p] = block[whaterver];
        	}
        }
        
        return[1] = tmp_1;
        if return[0]:
        	return[2] = tmp_2;
        *************************************
        */
        sink(x_return(0, pos_file()[0]['num_LEDs'] == 2));
        
        tmp_1 = array(pos_file()[0]['num_samps']);
        if X_READ_SELF(0):
        	tmp_2 = array(pos_file()[0]['num_samps']);
        
        for(int i=1, p=0; i < pos_file().length; i++){
        	byte[] block = pos_file()[i];
        	for(int j=0;j < block/W;j++,p++){
        		tmp_1[p] = block[whaterver];
        		tmp_2[p] = block[whaterver];
        	}
        }
        
        sink(x_return(1, tmp_1));
        if X_READ_SELF(0):
        	sink(x_return(2, tmp_2));
    }
}


class xy_func {
private:
    both_xy_t both_xy(){
        return something;//TODO: this
    }
    
    template <typename T>
    yield_signal x_return(int n, T val){
        // TODO: store val
        return yield_signal(WRITTEN, n);
    }
    
    bool x_is_computed_self(int n){
        return true;// TODO: this        
    }

public:
    void operator()(sink_t& sink){
        /*
        If there are two LEDs, this takes both sets of XY data and combines them to get a single value of xy for
        each moment in time.  with 1 LED, this simply uses that 1 array of XY data.
        
        *************************************
        tmp = array(both_xy[1].length);
        if !both_xy[0]:
        	return both_xy[1]; // BTS-TODO: how do we handle this case, where the same underlying data is re-used at multiple points?
        						// explanation here is that if there is only 1 LED then no weighting is needed, so pass through the 1LED's data
        						// simplest thing would be to copy it, but that's a shame.
        for(ii=0;ii<tmp.length;ii+1)
        	tmp[ii] = both_xy[1][ii] *w_1 + both_xy[2][ii] *w_2;
        return tmp;
        *************************************
        */
        tmp = array(both_xy()[1].length);
        if !both_xy()[0]:
        	return both_xy()[1]; // BTS-TODO: how do we handle this case, where the same underlying data is re-used at multiple points?
        						// explanation here is that if there is only 1 LED then no weighting is needed, so pass through the 1LED's data
        						// simplest thing would be to copy it, but that's a shame.
        for(ii=0;ii<tmp.length;ii+1)
        	tmp[ii] = both_xy()[1][ii] *w_1 + both_xy()[2][ii] *w_2;
        return tmp;
    }
}


class dir_func {
private:
    both_xy_t both_xy(){
        return something;//TODO: this
    }
    xy_t xy(){
        return something;//TODO: this
    }
    pos_file_t pos_file(){
        return something;//TODO: this
    }
    
    template <typename T>
    yield_signal x_return(int n, T val){
        // TODO: store val
        return yield_signal(WRITTEN, n);
    }
    
    bool x_is_computed_self(int n){
        return true;// TODO: this        
    }

public:
    void operator()(sink_t& sink){
        /*
        The first element is a flag, inidicating whether two LEDs were used.
        The second element is always dir displacement, and the third value
        is the "true" direction if 2 LEDs are avaialble.
        
        *************************************
        return[0] = both_xy[0]; // copying the bool across is just for convenience really
        
        if (1 in requested_blocks){
        	// compute dir_disp form xy
        	tmp = array(both_xy[1].length);
        	for(ii=0;ii<tmp.length;ii+1)
        		tmp[ii] = atan2(xy[ii].x-xy[ii-1].x, xy[ii].y-xy[ii-1].y);
        	return[1] = tmp;
        }
        
        if (both_xy[0] && 2 in requested_blocks){
        	int adjustment = pos_file[0]['LED_alignment']
        	tmp = array(both_xy[1].length);
        	for(ii=0;ii<tmp.length;ii+1)
        		tmp[ii] = atan2(both_xy[2][ii].x-both_xy[1][ii].x, both_xy[2][ii].y-both_xy[1][ii].y) + adjustment;
        	return[2] = tmp;
        }
        *************************************
        */
        sink(x_return(0, both_xy()()[0])); // copying the bool across is just for convenience really
        
        if (1 in requested_blocks){
        	// compute dir_disp form xy()
        	tmp = array(both_xy()()[1].length);
        	for(ii=0;ii<tmp.length;ii+1)
        		tmp[ii] = atan2(xy()[ii].x-xy()[ii-1].x, xy()[ii].y-xy()[ii-1].y);
        	sink(x_return(1, tmp));
        }
        
        if (both_xy()()[0] && 2 in requested_blocks){
        	int adjustment = pos_file()[0]['LED_alignment']
        	tmp = array(both_xy()()[1].length);
        	for(ii=0;ii<tmp.length;ii+1)
        		tmp[ii] = atan2(both_xy()()[2][ii].x-both_xy()()[1][ii].x, both_xy()()[2][ii].y-both_xy()()[1][ii].y) + adjustment;
        	sink(x_return(2, tmp));
        }
    }
}


class speed_func {
private:
    xy_t xy(){
        return something;//TODO: this
    }
    pos_file_t pos_file(){
        return something;//TODO: this
    }
    
    template <typename T>
    yield_signal x_return(int n, T val){
        // TODO: store val
        return yield_signal(WRITTEN, n);
    }
    
    bool x_is_computed_self(int n){
        return true;// TODO: this        
    }

public:
    void operator()(sink_t& sink){
        /*
        
        
        *************************************
        f = pos_file[0]['timebase'];
        tmp = array(xy.length);
        for(ii=0;ii<tmp.length;ii++)
        	tmp[ii] = hypot(xy[ii].x-xy[ii-1].x, xy[ii].y-xy[ii-1].y) * f;
        return tmp;
        *************************************
        */
        f = pos_file()[0]['timebase'];
        tmp = array(xy().length);
        for(ii=0;ii<tmp.length;ii++)
        	tmp[ii] = hypot(xy()[ii].x-xy()[ii-1].x, xy()[ii].y-xy()[ii-1].y) * f;
        return tmp;
    }
}


class dist_to_boundary_func {
private:
    xy_t xy(){
        return something;//TODO: this
    }
    boundary_shape_t boundary_shape(){
        return something;//TODO: this
    }
    
    template <typename T>
    yield_signal x_return(int n, T val){
        // TODO: store val
        return yield_signal(WRITTEN, n);
    }
    
    bool x_is_computed_self(int n){
        return true;// TODO: this        
    }

public:
    void operator()(sink_t& sink){
        /*
        
        
        *************************************
        // might want to implement it as dist_to_boundary squared..although that only makes sense for circle not square 
        // so the details are a bit more complicated.
        float[] tmp = array(xy.length);
        if(boundary_shape.kind == circle){
        	for(int i=0;i<tmp.length;i++)
        		tmp[i] = hypot(xy[i].x-boundary_shape.shape.centre.x, xy[i].y-boundary_shape.shape.centre.y);
        }else{ // boundary_shape.kind == rect
        	for(int i=0;i<tmp.length; i++){
        		tmp[i] = boundary_dist_rect(xy[i], boundary_shape.shape.topleft, boundary_shape.shape.W, boundary_shape.shape.H);
        	}
        }
        return tmp;
        *************************************
        */
        // might want to implement it as dist_to_boundary squared..although that only makes sense for circle not square 
        // so the details are a bit more complicated.
        float[] tmp = array(xy().length);
        if(boundary_shape().kind == circle){
        	for(int i=0;i<tmp.length;i++)
        		tmp[i] = hypot(xy()[i].x-boundary_shape().shape.centre.x, xy()[i].y-boundary_shape().shape.centre.y);
        }else{ // boundary_shape().kind == rect
        	for(int i=0;i<tmp.length; i++){
        		tmp[i] = boundary_dist_rect(xy()[i], boundary_shape().shape.topleft, boundary_shape().shape.W, boundary_shape().shape.H);
        	}
        }
        return tmp;
    }
}


class pos_mask_func {
private:
    pos_file_t pos_file(){
        return something;//TODO: this
    }
    trial_time_slice_t trial_time_slice(){
        return something;//TODO: this
    }
    directional_slice_t directional_slice(){
        return something;//TODO: this
    }
    dir_t dir(){
        return something;//TODO: this
    }
    spatial_mask_t spatial_mask(){
        return something;//TODO: this
    }
    boundary_dist_slice_t boundary_dist_slice(){
        return something;//TODO: this
    }
    dist_to_boundary_t dist_to_boundary(){
        return something;//TODO: this
    }
    
    template <typename T>
    yield_signal x_return(int n, T val){
        // TODO: store val
        return yield_signal(WRITTEN, n);
    }
    
    bool x_is_computed_self(int n){
        return true;// TODO: this        
    }

public:
    void operator()(sink_t& sink){
        /*
        True means use pos, False means don't use it.
        Computes a mask using zero or more of four differnt possible speicifications.
        Check [0] to see if the mask is all-False, or all-True.  
        Note that you might want to have something even more complciated in terms of adding, subtracting and multiple slices.
        Not sure how easy that would be to implement in any framework, psoup or otherwise...well it might be ok in python actually.
        
        *************************************
        if(!isnull(trial_time_slice)  ||
           !isnull(directional_slice) ||
           !isnull(spatial_mask)	  ||
           !isnull(boundary_dist_slice)){
        
           	   // some kind of masking has been requested
           	   if(!computed(return[1])){
        		   // we may already have computed 1, (in which case we are supposed to compute the logical_summary 0)
        		   	   	   
        		   vector<bool> tmp = array(pos_file[0]["num_samps"]);
        		   
        		   if(!isnull(trial_time_slice)){
        		   		tmp[:trial_time_slice.start = False;
        		   		tmp[trial_time_slice.end:] = False;
        		   }
        
        		   if(!isnull(directional_slice)){
        				tmp[!(directional_slice.start < dir[1] < directional_slice.end)] = False;
        		   }
        
        		   if(!isnull(boundary_dist_slice)){
        				tmp[!(boundary_dist_slice.start < dist_to_boundary < boundary_dist_slice.end)] = False;
        		   }
        
        		   //TODO: implement spatial mask
        
        		   return[1] = tmp;
        
        	   }
        	   if(0 in requested_blocks){
        	   		if(all(return[1]))
        	   			return[0] = all;
        	   		else if(none(return([1])))
        	   			return[0] = none;
        	   		else
        	   			return[0] = mixture;
        	   }
        
           }else{
        	   return[0] = all; //mask is all-true, so don't actually need to make it
           }
        *************************************
        */
        if(!isnull(trial_time_slice())  ||
           !isnull(dir()ectional_slice()) ||
           !isnull(spatial_mask())	  ||
           !isnull(boundary_dist_slice())){
        
           	   // some kind of masking has been requested
           	   if(!x_is_computed_self(1)){
        		   // we may already have computed 1, (in which case we are supposed to compute the logical_summary 0)
        		   	   	   
        		   vector<bool> tmp = array(pos_file()[0]["num_samps"]);
        		   
        		   if(!isnull(trial_time_slice())){
        		   		tmp[:trial_time_slice().start = False;
        		   		tmp[trial_time_slice().end:] = False;
        		   }
        
        		   if(!isnull(dir()ectional_slice())){
        				tmp[!(dir()ectional_slice().start < dir()[1] < dir()ectional_slice().end)] = False;
        		   }
        
        		   if(!isnull(boundary_dist_slice())){
        				tmp[!(boundary_dist_slice().start < dist_to_boundary() < boundary_dist_slice().end)] = False;
        		   }
        
        		   //TODO: implement spatial mask
        
        		   sink(x_return(1, tmp));
        
        	   }
        	   if(0 in requested_blocks){
        	   		if(all(X_READ_SELF(1)))
        	   			sink(x_return(0, all));
        	   		else if(none(return([1])))
        	   			sink(x_return(0, none));
        	   		else
        	   			sink(x_return(0, mixture));
        	   }
        
           }else{
        	   sink(x_return(0, all)); //mask is all-true, so don't actually need to make it
           }
    }
}


class speed_dwell_func {
private:
    xy_t xy(){
        return something;//TODO: this
    }
    pos_mask_t pos_mask(){
        return something;//TODO: this
    }
    speed_bin_size_t speed_bin_size(){
        return something;//TODO: this
    }
    
    template <typename T>
    yield_signal x_return(int n, T val){
        // TODO: store val
        return yield_signal(WRITTEN, n);
    }
    
    bool x_is_computed_self(int n){
        return true;// TODO: this        
    }

public:
    void operator()(sink_t& sink){
        /*
        
        
        *************************************
        if(pos_mask[0] == all)
        	return hist(xy, speed_bin_size);
        else
        	return hist_masked(xy, pos_mask, speed_bin_size);
        *************************************
        */
        if(pos_mask()[0] == all)
        	return hist(xy(), speed_bin_size());
        else
        	return hist_masked(xy(), pos_mask(), speed_bin_size());
    }
}