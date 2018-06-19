/*
Name:
	Final Game
Description:
	Structures, Functions for setting up a game quickly.
	Also declares extern functions for init/kill/update/render a game instance.

	This file is part of the final_framework.
License:
	MIT License
	Copyright 2018 Torsten Spaete
*/

#ifndef FINAL_GAME_H
#define FINAL_GAME_H

#if !(defined(__cplusplus) && ((__cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1900)))
#error "C++/11 compiler not detected!"
#endif

#include "final_math.h"
#include "final_render.h"

struct ButtonState {
	bool isDown;
	int halfTransitionCount;
};

inline bool WasPressed(const ButtonState &state) {
	bool result = ((state.halfTransitionCount > 1) || ((state.halfTransitionCount == 1) && (state.isDown)));
	return(result);
}

struct Controller {
	bool isConnected;
	bool isAnalog;
	Vec2f analogMovement;
	union {
		struct {
			ButtonState moveUp;
			ButtonState moveDown;
			ButtonState moveLeft;
			ButtonState moveRight;
			ButtonState actionUp;
			ButtonState actionDown;
			ButtonState actionLeft;
			ButtonState actionRight;
			ButtonState actionBack;
			ButtonState debugToggle;
		};
		ButtonState buttons[10];
	};
};

struct Mouse {
	Vec2i pos;
	float wheelDelta;
	union {
		struct {
			ButtonState left;
			ButtonState middle;
			ButtonState right;
		};
		ButtonState buttons[3];
	};
};

struct Input {
	float deltaTime;
	union {
		struct {
			Controller keyboard;
			Controller gamepad[4];
		};
		Controller controllers[5];
	};
	Mouse mouse;
	Vec2i windowSize;
	int defaultControllerIndex;
	bool isActive;
};

struct GameMemory {
	void *base;
	size_t capacity;
	size_t used;
};

extern GameMemory GameCreate();
extern void GameDestroy(GameMemory &gameMemory);
extern void GameInput(GameMemory &gameMemory, const Input &input);
extern void GameUpdate(GameMemory &gameMemory, const Input &input);
extern void GameRender(GameMemory &gameMemory, CommandBuffer &renderCommands, const float alpha, const float deltaTime);
extern void GameUpdateAndRender(GameMemory &gameMemory, const Input &input, CommandBuffer &renderCommands, const float alpha);
extern bool IsGameExiting(GameMemory &gameMemory);

#endif // FINAL_GAME_H