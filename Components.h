#pragma once
#include <memory>

struct Transform
{
	olc::vf2d position;
	float rotation;
	olc::vf2d scale;
};

struct Gravity
{
	olc::vf2d force;
};

struct RigidBody
{
	olc::vf2d velocity;
	olc::vf2d acceleration;
};

struct Graphic
{
	std::shared_ptr<olc::Decal> decal;
	olc::Pixel tint;
};

struct Input
{
	float speed;
};

struct Collision
{
	float radius;
	olc::vf2d center;
	bool isCollision;
};

struct Bullet
{
	olc::vf2d velocity;
};

struct AI
{
	olc::vf2d velocity;
	float shootInterval;
	float shootTimer;
};