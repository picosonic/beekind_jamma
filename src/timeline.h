// Timeline object

struct timelineitem
{
	uint64_t frame; // Frame count to execute this item
	void (*func)(); // Function to run
	bool done; // If this has been run
};

struct timelineobj
{
	std::vector<struct timelineitem> timeline; // Array of actions
	uint64_t timelinepos; // Current frame counter
	void (*callback)(float); // Optional callback on each timeline "tick"
	bool running; // Start in non-running state
	uint64_t looped; // Completed iterations
	uint64_t loop; // Number of times to loop, 0 means infinite
};

struct timelineobj tl;

// Add a new function to timeline with a given start time
void
timeline_add(const uint64_t itemstart, void (*newitem)())
{
	struct timelineitem item;

	item.frame=itemstart;
	item.func=newitem;
	item.done=false;

	tl.timeline.push_back(item);

	// TODO - sort timeline into frame-stamp order ??
}

// Add a timeline callback
void
timeline_addcallback(void (*callback)(float))
{
	tl.callback=callback;
}

// Called once per frame
void
timeline_call()
{
	uint64_t remain=0; // Tasks left to run, to determine when we are done

	// Stop further processing if we're not running
	if (!tl.running) return;

	// Look through timeline array for jobs not run which should have
	for (uint64_t i=0; i<tl.timeline.size(); i++)
	{
		if ((!tl.timeline[i].done) && (tl.timeline[i].frame<=tl.timelinepos))
		{
			tl.timeline[i].done=true;

			// Only call function if it is defined
			if (tl.timeline[i].func!=NULL)
				tl.timeline[i].func();
		}

		// Keep a count of all remaining jobs if still running
		if ((tl.running==true) && ((tl.timeline.size()-1)>=i) && (!tl.timeline[i].done))
			remain++;
	}

	// If a callback was requested, then call it
	if (tl.callback!=NULL)
	{
		// If there's only a single NULL function on the timeline and it doesn't start at 0, then call with percentage
		if ((tl.timeline.size()==1) && (tl.timeline[0].func==NULL) && (tl.timeline[0].frame>0))
			tl.callback(((float)tl.timelinepos/(float)tl.timeline[0].frame)*100); // percentage complete
		else
			tl.callback(0.0);
	}

	// Check for timeline being complete
	if (remain==0)
	{
		tl.looped++;

		// Check for more iterations required
		if ((tl.loop==0) || (tl.looped<tl.loop))
		{
			tl.timelinepos=0;

			// Look through timeline, resetting done flag
			for (uint64_t j=0; j<tl.timeline.size(); j++)
				tl.timeline[j].done=false;
		}
		else
			tl.running=false;
	}

	tl.timelinepos++;
}

// Start the timeline running
void
timeline_begin(const uint64_t loops)
{
	tl.looped=0;
	tl.loop=loops;
	tl.timelinepos=0;

	tl.running=true;
}

// Stop the timeline running
void
timeline_end()
{
	tl.running=false;
}

// Reset the timeline to be used again
void
timeline_reset()
{
	tl.running=false; // Start in non-running state

	tl.timeline.clear(); // Array of actions
	tl.timelinepos=0; // Frame count
	tl.callback=NULL; // Optional callback on each timeline "tick"
	tl.looped=0; // Completed iterations
	tl.loop=1; // Number of times to loop, 0 means infinite
}
