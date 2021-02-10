
/**
 * This is a function that accepts a space invader's parameters such as:
 *
 * x - the x value of the invader's position
 * y - the y value of the invaders's current position
 * direction - an int, indicating the direction of the invader (0 - right, 1 - left)
 * xstart - the invader's predetermined left boundary (x value)
 * ystart - the invader's predetermined left boundary (y value)
 * xend = the invader's predetermined right boundary (x value)
 * yend - the invader's predetermined right boundary (y value)
 *   
 * And, updates the coordinates and direction of the given cloud appropriately.
 */
function updateInvader(id, x, y, direction, xstart,  ystart,  xend, yend)
{
	var min = x - 1;
	var max = x + 1;
	// direction is moving right 
	if (direction == 0) {
		if ( max >= xend) {
			direction = 1;
			setSpaceInvadersDirection(id, 1);
			y+=20;
		} else {
			x+=5;
		}
	// direction is moving left	
	} else {
		if ( min <= xstart) {
			direction = 0;
			setSpaceInvadersDirection(id, 0);
			y+=20;
		} else {
			x-=5;
		}
	
	}
	setSpaceInvadersCoords(id, x, y);
	return "Update Invader Position for Id " + id + ".  X is " + x +  "  & Y is " + y + ".";
}

