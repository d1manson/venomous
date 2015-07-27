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


float angle_ab(point a, point b){
	auto dx = b.x - a.x;
	auto dy = b.y - a.y;
	return abs(dx) > 0.001 && abs(dy) > 0.001 ? atan2(dy,dx) : nan;
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

logical_summary make_logical_summary(bool[] X){
  		if(all(X))
  			return all;
  		else if(none(X))
  			return none;
  		else
  			return mixture;
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

struct speed_bin_size_t{
float 0_;
}

struct spa_bin_size_t{
float 0_;
}

struct cut_file_name_t{
axona_file_name_t _0;
}

struct tac_window_secs_t{
float 0_;
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
        if ( !computed(header) ){
        	// we may already have this cached.
        	map header_tmp;
        	// parse f into header, storing the n_block, and data_start pointer as well
        	header = header_tmp; 
        }
        
        for (block_ii in requested_blocks){
        	buffer[block_ii] = f.read(block_ii*BLOCK_SIZE, BLOCK_SIZE);
        }
        *************************************
        */
        const int BLOCK_SIZE = 2*1024*104; //2MB
        FileStreamer f(axona_file_name()); //this is C++ RAII in action, we are using axona_file_name() from its definition as an "input".
        int n_blocks = ceil(f.length()/BLOCK_SIZE); 
        if ( !computed(header) ){
        	// we may already have this cached.
        	map header_tmp;
        	// parse f into header, storing the n_block, and data_start pointer as well
        	header = header_tmp; 
        }
        
        for (block_ii in requested_blocks){
        	buffer[block_ii] = f.read(block_ii*BLOCK_SIZE, BLOCK_SIZE);
        }
    }
}


class both_xy_func {
private:
    pos_file_t pos_file(){
        return something;//TODO: this
    }
    set_file_t set_file(){
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
        used_both = set_file.header['colactive_2']; 
        
        int num_samps = parseInt(pos_file.header['num_samps']);
        if (use_both){
        	xy1.allocate(num_samps);
        	xy2.allocate(num_samps);
        	for(auto c pos_file.buffer.iter_blocks(16)){
        		xy1.write(block[3]); // TODO: actual post processing..this is complicated for a number of reasons.
        		xy2.write(block[4]);
        	}	
        }else{
        	xy_1.allocate(num_samps);
        	for(auto c pos_file.buffer.iter_blocks(16))
        		xy1.write(block[3]);
        }
        w1 = sum(!nan(xy1));
        w2 = sum(!nan(xy2));
        *************************************
        */
        used_both = set_file().header['colactive_2']; 
        
        int num_samps = parseInt(pos_file().header['num_samps']);
        if (use_both){
        	xy1.allocate(num_samps);
        	xy2.allocate(num_samps);
        	for(auto c pos_file().buffer.iter_blocks(16)){
        		xy1.write(block[3]); // TODO: actual post processing..this is complicated for a number of reasons.
        		xy2.write(block[4]);
        	}	
        }else{
        	xy_1.allocate(num_samps);
        	for(auto c pos_file().buffer.iter_blocks(16))
        		xy1.write(block[3]);
        }
        w1 = sum(!nan(xy1));
        w2 = sum(!nan(xy2));
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
        if(!both_xy.used_both){
        	xy = both_xy.xy1;
        }else{
        	xy.allocate(both_xy.xy1.length);
        	for(auto p1, p2 : both_xy.xy1, both_xy.xy2)
        		xy.write(p1*both_xy.w1 + p2*both_xy.w2);
        }
        *************************************
        */
        if(!both_xy().used_both){
        	xy = both_xy().xy1;
        }else{
        	xy.allocate(both_xy().xy1.length);
        	for(auto p1, p2 : both_xy().xy1, both_xy().xy2)
        		xy.write(p1*both_xy().w1 + p2*both_xy().w2);
        }
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
    set_file_t set_file(){
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
        The used_both is copied across from both_xy for convenience.
        If both LEDs are used then dir!= dir_disp, otherwise dir is dir_disp.
        
        *************************************
        used_both = both_xy.used_both;
        dir_disp.allocate(xy.length);
        if(used_both){ 
        	// TODO: could check which of the two dirs is requested and do different loop based on that. requested(dir_disp)
        	dir.allocate(xy.length);
        	point pw_old(nan,nan); 
        	for(auto p1, p2, pw : both_xy.xy1, both_xy.xy2, xy){
        		dir.write(angle_ab(p1,p2));
        		dir_disp.write(angle_ab(pw,pw_old));
        		pw_old = pw;	
        	}
        }else{
        	dir_disp.allocate(xy.length);
        	dir.equals(dir_disp);
        	point pw_old(nan,nan); 
        	for(auto pw : xy){
        		dir_disp.write(angle_ab(pw,pw_old));
        		pw_old = pw;	
        	}
        }
        *************************************
        */
        used_both = both_xy()().used_both;
        dir_disp.allocate(xy().length);
        if(used_both){ 
        	// TODO: could check which of the two dirs is requested and do different loop based on that. requested(dir_disp)
        	dir.allocate(xy().length);
        	point pw_old(nan,nan); 
        	for(auto p1, p2, pw : both_xy()().xy()1, both_xy()().xy()2, xy()){
        		dir.write(angle_ab(p1,p2));
        		dir_disp.write(angle_ab(pw,pw_old));
        		pw_old = pw;	
        	}
        }else{
        	dir_disp.allocate(xy().length);
        	dir.equals(dir_disp);
        	point pw_old(nan,nan); 
        	for(auto pw : xy()){
        		dir_disp.write(angle_ab(pw,pw_old));
        		pw_old = pw;	
        	}
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
        int f = parseInt(pos_file.header['timebase']);
        speed.allocate(xy.length);
        point p_old (nan,nan);
        for(auto p : xy){
        	speed.write(hypot(p_old.x - p.x, p_old.y - p.y) * f);
        	p_old = p;
        }
        *************************************
        */
        int f = parseInt(pos_file().header['timebase']);
        speed.allocate(xy().length);
        point p_old (nan,nan);
        for(auto p : xy()){
        	speed.write(hypot(p_old.x - p.x, p_old.y - p.y) * f);
        	p_old = p;
        }
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
        dist_to_boundary.allocate(xy.length);
        if(boundary_shape.kind == circle){
        	for(auto p : xy)
        		dist_to_boundary.write( hypot(p.x-boundary_shape.shape.centre.x, p.y-boundary_shape.shape.centre.y));
        }else{ // boundary_shape.kind == rect
        	for(auto p : xy)
        		dist_to_boundary.write( boundary_dist_rect(p, boundary_shape.shape.topleft, boundary_shape.shape.W, boundary_shape.shape.H));
        }
        *************************************
        */
        // might want to implement it as dist_to_boundary squared..although that only makes sense for circle not square 
        // so the details are a bit more complicated.
        dist_to_boundary.allocate(xy().length);
        if(boundary_shape().kind == circle){
        	for(auto p : xy())
        		dist_to_boundary.write( hypot(p.x-boundary_shape().shape.centre.x, p.y-boundary_shape().shape.centre.y));
        }else{ // boundary_shape().kind == rect
        	for(auto p : xy())
        		dist_to_boundary.write( boundary_dist_rect(p, boundary_shape().shape.topleft, boundary_shape().shape.W, boundary_shape().shape.H));
        }
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
        Check summary to see if the mask is all-False, or all-True.  
        Note that you might want to have something even more complciated in terms of adding, subtracting and multiple slices.
        Not sure how easy that would be to implement in any framework, psoup or otherwise...well it might be ok in python actually.
        
        *************************************
        if(!isnull(trial_time_slice)  ||
           !isnull(directional_slice) ||
           !isnull(spatial_mask)	  ||
           !isnull(boundary_dist_slice)){
        
           	   // some kind of masking has been requested
           	   if(!computed(mask)){
        		   // we may already have computed mask, (in which case we are supposed to compute summary)
        		   	   	   
        		   vector<bool> tmp = array(pos_file.header["num_samps"]);
        		   
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
        
        		   mask = tmp;
        
        	   }
        	   if(requested(summary))
        	   		summary = make_logical_summary(mask);
        
           }else{
        	   summary = all; //mask is all-true, so don't actually need to make it
           }
        *************************************
        */
        if(!isnull(trial_time_slice())  ||
           !isnull(directional_slice()) ||
           !isnull(spatial_mask())	  ||
           !isnull(boundary_dist_slice())){
        
           	   // some kind of masking has been requested
           	   if(!computed(mask)){
        		   // we may already have computed mask, (in which case we are supposed to compute summary)
        		   	   	   
        		   vector<bool> tmp = array(pos_file().header["num_samps"]);
        		   
        		   if(!isnull(trial_time_slice())){
        		   		tmp[:trial_time_slice().start = False;
        		   		tmp[trial_time_slice().end:] = False;
        		   }
        
        		   if(!isnull(directional_slice())){
        				tmp[!(directional_slice().start < dir[1] < directional_slice().end)] = False;
        		   }
        
        		   if(!isnull(boundary_dist_slice())){
        				tmp[!(boundary_dist_slice().start < dist_to_boundary() < boundary_dist_slice().end)] = False;
        		   }
        
        		   //TODO: implement spatial mask
        
        		   mask = tmp;
        
        	   }
        	   if(requested(summary))
        	   		summary = make_logical_summary(mask);
        
           }else{
        	   summary = all; //mask is all-true, so don't actually need to make it
           }
    }
}


class spike_pos_inds_func {
private:
    pos_file_t pos_file(){
        return something;//TODO: this
    }
    spike_times_t spike_times(){
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
        // read timebases to get factor and apply to spike times
        *************************************
        */
        // read timebases to get factor and apply to spike times
    }
}


class spike_mask_func {
private:
    pos_mask_t pos_mask(){
        return something;//TODO: this
    }
    spike_pos_inds_t spike_pos_inds(){
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
        //lookup spike pos inds in pos mask..use sorted_access_iterator
        *************************************
        */
        //lookup spike pos inds in pos mask..use sorted_access_iterator
    }
}


class speed_dwell_func {
private:
    speed_t speed(){
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
        if(pos_mask.summary == all)
        	speed_dwell = hist(speed, speed_bin_size);
        else
        	speed_dwell = hist_masked(speed, pos_mask, speed_bin_size);
        *************************************
        */
        if(pos_mask().summary == all)
        	speed()_dwell = hist(speed(), speed()_bin_size);
        else
        	speed()_dwell = hist_masked(speed(), pos_mask(), speed()_bin_size);
    }
}


class pos_bin_ind_func {
private:
    spa_bin_size_t spa_bin_size(){
        return something;//TODO: this
    }
    xy_t xy(){
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
        pos_bin_ind.allocate(xy.length);
        for(auto p : xy)
        	pos_bin_ind.write({p.x/spa_bin_size, p.y/spa_bin_size});
        *************************************
        */
        pos_bin_ind.allocate(xy().length);
        for(auto p : xy())
        	pos_bin_ind.write({p.x/spa_bin_size(), p.y/spa_bin_size()});
    }
}


class spike_times_func {
private:
    tet_file_t tet_file(){
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
        returns spike times in same units as tet_file stored them
        
        *************************************
        timebase = parseInt(tet_file.header["timeabase"]);
        int n_spikes = parseInt(tet_file.header["nump_spikes"]);
        times.allocate(n_spikes);
        for(auto c : tet_file.buffer.iter_blocks(216));
        	times.write(c[0:4]);
        *************************************
        */
        timebase = parseInt(tet_file().header["timeabase"]);
        int n_spikes = parseInt(tet_file().header["nump_spikes"]);
        times.allocate(n_spikes);
        for(auto c : tet_file().buffer.iter_blocks(216));
        	times.write(c[0:4]);
    }
}


class cut_file_func {
private:
    cut_file_name_t cut_file_name(){
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
        FileStreamer f(cut_file_name);
        // read cut header, which gets discarded
        int n_spikes = ??? header val;
        cut_file.allocate(n_spikes);
        
        while(auto buffer = f.read(BLOCK_SIZE){ 
        	for(auto val in buffer.split('\n'))
        		cut_file.write(parseInt(val));
        }
        *************************************
        */
        FileStreamer f(cut_file_name());
        // read cut header, which gets discarded
        int n_spikes = ??? header val;
        cut_file.allocate(n_spikes);
        
        while(auto buffer = f.read(BLOCK_SIZE){ 
        	for(auto val in buffer.split('\n'))
        		cut_file.write(parseInt(val));
        }
    }
}