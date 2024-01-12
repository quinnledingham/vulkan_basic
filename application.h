struct App_Time {
	s64 start_ticks;
	s64 last_frame_ticks;
	s64 performance_frequency;

	float32 run_time_s;
	float32 frame_time_s;

	float32 frames_per_s;

	u32 avg_fps_sum_count;
	float32 avg_fps;
};

struct App {
	App_Time time;
};

inline float64
get_seconds_elapsed(s64 performance_frequency, s64 start, s64 end) {
    float64 result = ((float64)(end - start) / (float64)performance_frequency);
    return result;
}