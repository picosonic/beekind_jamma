struct pathnode
{
	int16_t id; // square on numbered grid
	int16_t x; // x position
	int16_t y; // y position
	int16_t p; // previous node that led here (or -1)
	float g; // cost to get here
	float h; // manhattan cost
	float f; // final cost
};

// A* algorithm from pseudocode in Wireframe magazine issue 48
// by Paul Roberts
std::vector<int16_t>
pathfinder(const int16_t src, const int16_t dest)
{
	std::vector<struct pathnode> openlist; // List of node ids yet to visit
	std::vector<struct pathnode> closedlist; // List of visited node ids yet

	const int16_t dx=Math_floor(dest%gs.width); // Destination node X grid position
	const int16_t dy=Math_floor(dest/gs.width); // Destination node Y grid position

	struct pathnode n; // Next node
	const int16_t nx=Math_floor(src%gs.width); // Next node X grid position
	const int16_t ny=Math_floor(src/gs.width); // Next node Y grid position

	int16_t c=-1; // Check node id
	int16_t cx=-1; // Check node X grid position
	int16_t cy=-1; // Check node Y grid position

	// Check if this grid position is solid (out of bounds to path)
	auto issolid = [&](const int16_t x, const int16_t y)
	{
		// Out of bounds check
		if ((x<0) || (x>=gs.width) || (y<0) || (y>=gs.height))
			return true;

		// Solid check
		return (levels[gs.level].tiles[(y*gs.width)+x]!=0);
	};

	// Determine cost (rough distance) from x1,y1 to x2,y2
	auto manhattan_cost = [](const int16_t x1, const int16_t y1, const int16_t x2, const int16_t y2)
	{
		return (abs(x1-x2)+abs(y1-y2));
	};

	// Add node to open list
	auto addnode = [&](const int16_t id, const int16_t x, const int16_t y, const int16_t prev, const float acc)
	{
		float g=acc; // Cost to get here
		float h=manhattan_cost(x, y, dx, dy);
		float f=g+h; // Final cost
		struct pathnode node;

		node.id=id;
		node.x=x;
		node.y=y;
		node.p=prev;
		node.g=g;
		node.h=h;
		node.f=f;
  
		openlist.push_back(node);
	};

	// Find the node on the open list with the lowest value
	auto findcheapestopenlist = [&]()
	{
		float cost=-1;
		int16_t idx=0;
  
		for (uint16_t i=0; i<openlist.size(); i++)
		{
			if ((cost==-1) || (openlist[i].f<cost))
			{
				cost=openlist[i].f;
				idx=i;
			}
		}
  
		return openlist[idx];
	};

/*
	// Get index to node id on given list
	auto getidx = [](const std::vector<struct pathnode> & givenlist, const int16_t id)
	{
		for (int16_t i=0; i<givenlist.size(); i++)
			if (givenlist[i].id==id) return (int16_t)i;

		return -1;
	};
*/

	// Is given node id on the openlist
	auto isopen = [&](const int16_t id)
	{
		for (uint32_t i=0; i<openlist.size(); i++)
			if (openlist[i].id==id) return true;

		return false;
	};

	// Is given node id on the closedlist
	auto isclosed = [&](const int16_t id)
	{
		for (uint32_t i=0; i<closedlist.size(); i++)
			if (closedlist[i].id==id) return true;

		return false;
	};

	// Move given node from open to closed list
	auto movetoclosedlist = [&](const int16_t id)
	{
		// Find id in openlist list
		for (uint32_t i=0; i<openlist.size(); i++)
			if (openlist[i].id==id)
			{
				closedlist.push_back(openlist[i]);
				openlist.erase(openlist.begin()+i);
  
				return;
			}
	};

	// Find parent of given node, to backtrace path
	auto findparent = [&](const int16_t id)
	{
		uint32_t i;
  
		for (i=0; i<closedlist.size(); i++)
			if (closedlist[i].id==id)
				return closedlist[i].p;
  
		for (i=0; i<openlist.size(); i++)
			if (openlist[i].id==id)
				return openlist[i].p;
  
		return (int16_t)-1;
	};

	// Retrace path back to start position
	auto retracepath = [&](const int16_t dest)
	{
		std::vector<int16_t> finalpath;

		// Check for path being found
		if (n.id==dest)
		{
			int16_t prev=findparent(dest);
			finalpath.insert(finalpath.begin(), dest);

			while (prev!=-1)
			{
				finalpath.insert(finalpath.begin(), prev);

				prev=findparent(prev);
			}
		}

		return finalpath;
	};

	auto explore = [&](const int16_t x, const int16_t y)
	{
		c=n.id+(x+(y*gs.width));
		cx=n.x+x;
		cy=n.y+y;

		// If it's a valid square, not solid, nor on any list, then add it
		if (!issolid(cx, cy) && (!isopen(c)) && (!isclosed(c)))
			addnode(c, cx, cy, n.id, n.f+1); // Add to open list

		// NOTE : No cheaper path replacements done. This is when a cheaper path
		// is found to get to a certain point already visited. If found the costs
		// and parent data should be updated.
	};

	// Add source to openlist list
	addnode(src, nx, ny, -1, 0);
	n=findcheapestopenlist();

	// While openlist list has nodes to search
	while ((n.id!=dest) && (openlist.size()>0))
	{
		// Set n to cheapest node from openlist list
		n=findcheapestopenlist();

		// Check if n is the target node
		if (n.id==dest) break;

		// Check for unexplored nodes connecting to n
		explore( 0, -1); // Above
		explore( 1,  0); // Right
		explore( 0,  1); // Below
		explore(-1 , 0); // Left

		// Move n to the closedlist list
		movetoclosedlist(n.id);
	}

	return retracepath(dest);
}
