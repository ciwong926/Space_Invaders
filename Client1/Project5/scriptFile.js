/** 
 * A Key to the different Event Types:
 *
 * CHARACTER_DEATH = 4,// When the character dies 
 * USER_INPUT = 8, // When the user clicks something (&  its not related to recording)
 * CLIENT_CONNECT = 11, // When the user attempts to end a recording
 * RESTART = 12
 */

 /**
  * Different Types Of Keyboard input:
  * 
  * ARROW = 0, A = 1, B = 2, C = 3, D = 4, E = 5, F = 6, G = 7, H = 8, I = 9, 
  * J = 10, K = 11, L = 12, M = 13, N = 14, O = 15, P = 16, Q = 17, R = 18, S = 19, 
  * T = 20, U = 21, V = 22, W = 23, X = 24, Y = 25, Z = 26, Shift = 27, Space = 28
  */

/**
 * Diffent Types of Actions:
 *
 * STILL = 0, FALL = 1, JUMP = 2, LEFT = 3, RIGHT = 4, UP = 5, DOWN = 6, PUSH = 7, DIE = 8, 
 * SCROLL_RIGHT = 9, SCROLL_LEFT = 10
 */

 /*
  * This is a function that reacts to user input. This includes creating raising events
  * based on the type of user input.
  * 
  * Keys are numerical. This is a key:
  * Key 0 = Right
  * Key 1 = Left 
  * Key 2 = Up
  */
function reactToKeyboard(key) {

	// Reacting To Right Key Press
	if (key == 0) {

		var strings = [];
		strings[0] = "key";
		strings[1] = "type";
		strings[2] = "valid";

		var nums = [];
		nums[0] = 0; // Arrow = 0
		nums[1] = 4; // Right = 4
		nums[2] = 0; // Vadility ?

		var num = getCharacterX();
		
		if (getCharacterX() < 725) {
		    nums[2] = 1;
		}
		// Create Raise Event Constructor = (int type, int priority, int args, std::vector<std::string> keys, std::vector<int> values);
		createRaiseEvent(8, 3, 3, strings, nums); // 8 = User Input 

	}

	// Reacting To Left Key Press
	if (key == 1) {
		var strings = [];
		strings[0] = "key";
		strings[1] = "type";
		strings[2] = "valid";

		var nums = [];
		nums[0] = 0; // Arrow = 0
		nums[1] = 3; // Left = 3
		nums[2] = 0; // Vadility ?
		
		if (getCharacterX() > 25) {
		    nums[2] = 1;
		} else {

			var strings2 = [];
			strings2[0] = "action"; // the character collision action
			strings2[1] = "auto"; // auto = is this an automatic update?

			var nums2 = [];
			nums2[0] = getLeftAction(); // TBD
			nums2[1] = 0; // false

			createRaiseEvent(1, 4, 2, strings2, nums2); // 1 = Character Collision
		}

		// Create Raise Event Constructor = (int type, int priority, int args, std::vector<std::string> keys, std::vector<int> values);
		createRaiseEvent(8, 3, 3, strings, nums); // 8 = User Input 
	}

	// Reacting To Up Key Press
	if (key == 2) {

		var strings = [];
		strings[0] = "key";
		strings[1] = "type";
		strings[2] = "valid";

		var nums = [];
		
		nums[0] = 0; // Arrow = 0
		nums[1] = 2; // Jump = 2
		nums[2] = 1; // Vadility ?

		// Create Raise Event Constructor = (int type, int priority, int args, std::vector<std::string> keys, std::vector<int> values);
		createRaiseEvent(8, 1, 3, strings, nums); // 8 = User Input 
	} 

	return "Success";
}

/**
 * Finds the value for the key in the 2D array.
 */
function findArgValue(arr, key) {
	for (var i = 0; i < arr.length; i++) {
		if (arr[i][0].localeCompare(key) == 0) {
			return arr[i][1];
		}
	}
}

/** 
 * This is a class that retrieves event data via a string and determines and 
 * acts on the worlds next move as a result. 
 */
function handleWorldEvent(message)
{
	// printMessage("GOT HERE");
	// Get Array, Event Type & Num Of Arguments
	var eventArr = message.split(",");
	var type = eventArr[0]; // The Type of Event 
	var args = eventArr[3]; // The Number Of Arguments The Event Contains

	// A 2D Array of Arguments
	var argMap = [];
	var buff = 0;
	for (var i = 0; i < args; i++) {
		argMap[i] = [eventArr[4+buff], eventArr[6+buff]];
		buff += 3;
	}

	// If The Event Type Is RESTART
	if (parseInt(type) == 12) {
		restart();
    }

	return "Success";
}


