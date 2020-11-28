#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "ECS.h"

Coordinator g_Coordinator;

void PhysicsSystem::Update(float deltaTime, olc::PixelGameEngine* engine)
{
	for (auto const& entity : m_Entities)
	{
		auto& rigidBody = g_Coordinator.GetComponent<RigidBody>(entity);
		auto& transform = g_Coordinator.GetComponent<Transform>(entity);
		auto const& gravity = g_Coordinator.GetComponent<Gravity>(entity);

		transform.position += rigidBody.velocity * deltaTime;

		rigidBody.velocity += gravity.force * deltaTime;

		if (transform.position.y <= -5.0f)
		{
			g_Coordinator.DestroyEntity(entity);
		}
	}
}

void RenderSystem::Render(olc::PixelGameEngine* engine)
{
	engine->Clear(olc::BLANK);
	for (auto const& entity : m_Entities)
	{
		auto& transform = g_Coordinator.GetComponent<Transform>(entity);
		auto& graphic = g_Coordinator.GetComponent<Graphic>(entity);
		auto& collision = g_Coordinator.GetComponent<Collision>(entity);

		graphic.tint = collision.isCollision ? olc::DARK_RED : olc::WHITE;
		engine->DrawDecal(transform.position, graphic.decal.get(), transform.scale, graphic.tint);

		//Draw collision circle
		//engine->DrawCircle(transform.position + collision.center, collision.radius, collision.isCollision ? olc::GREEN : olc::WHITE);
	}
}

void MovementSystem::OnMove(olc::vf2d direction)
{
	for (auto const& entity : m_Entities)
	{
		auto& transfrom = g_Coordinator.GetComponent<Transform>(entity);
		auto& input = g_Coordinator.GetComponent<Input>(entity);

		transfrom.position += direction * input.speed;
	}
}

void CollisionSystem::CheckCollision(Entity entity)
{
	auto& transform1 = g_Coordinator.GetComponent<Transform>(entity);
	auto& collision1 = g_Coordinator.GetComponent<Collision>(entity);

	for (auto const& other : m_Entities)
	{
		if (entity == other)
			continue;

		auto& transform2 = g_Coordinator.GetComponent<Transform>(other);
		auto& collision2 = g_Coordinator.GetComponent<Collision>(other);

		olc::vf2d pos1 = transform1.position;
		olc::vf2d pos2 = transform2.position;
		float radii = collision1.radius + collision2.radius;

		collision1.isCollision = ((pos1.x - pos2.x) * (pos1.x - pos2.x) +
								 (pos1.y - pos2.y) * (pos1.y - pos2.y)) <= radii * radii;

		collision2.isCollision = collision1.isCollision;

		if (collision1.isCollision)
		{
			break;
		}
	}
}

void CollisionSystem::CheckAllCollision()
{
	for (auto const& entity1 : m_Entities)
	{
		for (auto const& entity2 : m_Entities)
		{
			if (entity1 == entity2)
				continue;

			auto& transform1 = g_Coordinator.GetComponent<Transform>(entity1);
			auto& collision1 = g_Coordinator.GetComponent<Collision>(entity1);

			auto& transform2 = g_Coordinator.GetComponent<Transform>(entity2);
			auto& collision2 = g_Coordinator.GetComponent<Collision>(entity2);

			olc::vf2d pos1 = transform1.position;
			olc::vf2d pos2 = transform2.position;
			float radii = collision1.radius + collision2.radius;

			collision2.isCollision = ((pos1.x - pos2.x) * (pos1.x - pos2.x) +
									  (pos1.y - pos2.y) * (pos1.y - pos2.y)) <= radii * radii;

			collision1.isCollision = collision2.isCollision;

			if (collision1.isCollision)
			{
				return;
			}
		}
	}
}

void BulletSystem::MoveBullet(float deltaTime, olc::PixelGameEngine* engine, std::shared_ptr<CollisionSystem> collisionSystem)
{
	for (auto const& entity : m_Entities)
	{
		auto& transform = g_Coordinator.GetComponent<Transform>(entity);
		auto& bullet = g_Coordinator.GetComponent<Bullet>(entity);
		auto& collision = g_Coordinator.GetComponent<Collision>(entity);


		transform.position += bullet.velocity * deltaTime;
		collisionSystem->CheckCollision(entity);

		//Destroy bullet if collide or outside of game window
		if (collision.isCollision || transform.position.y < -collision.radius || transform.position.y > engine->ScreenHeight() + collision.radius)
		{
			g_Coordinator.DestroyEntity(entity);
		}
	}
}

void AISystem::Move(float deltaTime, olc::PixelGameEngine* engine, std::shared_ptr<CollisionSystem> collisionSystem)
{
	for (auto const& entity : m_Entities)
	{
		auto& transform = g_Coordinator.GetComponent<Transform>(entity);
		auto& ai = g_Coordinator.GetComponent<AI>(entity);
		auto& collision = g_Coordinator.GetComponent<Collision>(entity);


		transform.position += ai.velocity * deltaTime;
		collisionSystem->CheckCollision(entity);

		if (transform.position.y > engine->ScreenHeight() + collision.radius)
		{
			transform.position.y = -collision.radius * 2.0f;
		}

		//Destroy enemy if collide
		if (collision.isCollision)
		{
			g_Coordinator.DestroyEntity(entity);
		}
	}
}

void AISystem::Shoot(float deltaTime)
{
	for (auto const& entity : m_Entities)
	{
		auto& ai = g_Coordinator.GetComponent<AI>(entity);

		ai.shootTimer += deltaTime;

		if (ai.shootTimer >= ai.shootInterval)
		{
			auto& transform = g_Coordinator.GetComponent<Transform>(entity);
			auto& collision = g_Coordinator.GetComponent<Collision>(entity);

			olc::vf2d scale = olc::vf2d(0.1f, 0.1f);
			std::shared_ptr<olc::Decal> decal = std::make_shared<olc::Decal>(new olc::Sprite("BulletEnemy.png"));

			Entity entity = g_Coordinator.CreateEntity();
			g_Coordinator.AddComponent(entity, Transform{ .position = transform.position + olc::vf2d(5.0f, 10.0f), .scale = scale });
			g_Coordinator.AddComponent(entity, Graphic{ .decal = decal, .tint = olc::WHITE });
			g_Coordinator.AddComponent(entity, Bullet{ .velocity = olc::vf2d(0.0f, 100.0f) });
			g_Coordinator.AddComponent(entity, Collision{ .radius = 2.0f,
														  .center = olc::vf2d(decal.get()->sprite->width * 0.5f,
																			  decal.get()->sprite->height * 0.5f) * scale });

			ai.shootTimer -= ai.shootInterval;
		}
	}
}

class SpaceShooter : public olc::PixelGameEngine
{
public:
	SpaceShooter()
	{
		sAppName = "SpaceShooter";
	}

	std::shared_ptr<PhysicsSystem> physicsSystem;
	std::shared_ptr<RenderSystem> renderSystem;
	std::shared_ptr<MovementSystem> movementSystem;
	std::shared_ptr<CollisionSystem> collisionSystem;
	std::shared_ptr<BulletSystem> bulletSystem;
	std::shared_ptr<AISystem> aiSystem;

	olc::Sprite* shipSprite = nullptr;
	olc::Sprite* bulletSprite = nullptr;
	olc::Sprite* enemySprite = nullptr;

	float spawnInterval = 5.0f;
	float spawnTimer = spawnInterval;

public:
	bool OnUserCreate() override
	{
		// Called once at the start, so create things here

		shipSprite = new olc::Sprite("Ship.png");
		bulletSprite = new olc::Sprite("Bullet.png");
		enemySprite = new olc::Sprite("Enemy.png");

		g_Coordinator.Init();

		g_Coordinator.RegisterComponent<Gravity>();
		g_Coordinator.RegisterComponent<RigidBody>();
		g_Coordinator.RegisterComponent<Transform>();
		g_Coordinator.RegisterComponent<Graphic>();
		g_Coordinator.RegisterComponent<Input>();
		g_Coordinator.RegisterComponent<Collision>();
		g_Coordinator.RegisterComponent<Bullet>();
		g_Coordinator.RegisterComponent<AI>();

		physicsSystem = g_Coordinator.RegisterSystem<PhysicsSystem>();
		renderSystem = g_Coordinator.RegisterSystem<RenderSystem>();
		movementSystem = g_Coordinator.RegisterSystem<MovementSystem>();
		collisionSystem = g_Coordinator.RegisterSystem<CollisionSystem>();
		bulletSystem = g_Coordinator.RegisterSystem<BulletSystem>();
		aiSystem = g_Coordinator.RegisterSystem<AISystem>();

		Signature signature;
		signature.set(g_Coordinator.GetComponentType<Gravity>());
		signature.set(g_Coordinator.GetComponentType<RigidBody>());
		signature.set(g_Coordinator.GetComponentType<Transform>());
		g_Coordinator.SetSystemSignature<PhysicsSystem>(signature);

		signature.reset();
		signature.set(g_Coordinator.GetComponentType<Transform>());
		signature.set(g_Coordinator.GetComponentType<Graphic>());
		signature.set(g_Coordinator.GetComponentType<Collision>());
		g_Coordinator.SetSystemSignature<RenderSystem>(signature);

		signature.reset();
		signature.set(g_Coordinator.GetComponentType<Transform>());
		signature.set(g_Coordinator.GetComponentType<Input>());
		signature.set(g_Coordinator.GetComponentType<Collision>());
		g_Coordinator.SetSystemSignature<MovementSystem>(signature);

		signature.reset();
		signature.set(g_Coordinator.GetComponentType<Transform>());
		signature.set(g_Coordinator.GetComponentType<Collision>());
		g_Coordinator.SetSystemSignature<CollisionSystem>(signature);

		signature.reset();
		signature.set(g_Coordinator.GetComponentType<Transform>());
		signature.set(g_Coordinator.GetComponentType<Collision>());
		signature.set(g_Coordinator.GetComponentType<Bullet>());
		g_Coordinator.SetSystemSignature<BulletSystem>(signature);

		signature.reset();
		signature.set(g_Coordinator.GetComponentType<Transform>());
		signature.set(g_Coordinator.GetComponentType<Collision>());
		signature.set(g_Coordinator.GetComponentType<AI>());
		g_Coordinator.SetSystemSignature<AISystem>(signature);

		std::vector<Entity> entities(MAX_ENTITIES);

		olc::vf2d scale = olc::vf2d(0.1f, 0.1f);
		std::shared_ptr<olc::Decal> decal = std::make_shared<olc::Decal>(shipSprite);

		// Create player entity
		Entity entity = g_Coordinator.CreateEntity();
		g_Coordinator.AddComponent(entity, Transform{ .position = olc::vf2d(ScreenWidth() * 0.5f, ScreenHeight() * 0.8f -(decal.get()->sprite->height * scale.y)), .scale = scale });
		g_Coordinator.AddComponent(entity, Graphic{ .decal = decal, .tint = olc::WHITE });
		g_Coordinator.AddComponent(entity, Input{ .speed = 100.0f });
		g_Coordinator.AddComponent(entity, Collision{ .radius = 6.0f,
													  .center = olc::vf2d(decal.get()->sprite->width * 0.5f,
																		  decal.get()->sprite->height * 0.5f) * scale });

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		// called once per frame
		renderSystem->Render(this);

		//physicsSystem->Update(fElapsedTime, this);
		if (GetKey(olc::Key::UP).bHeld)
		{
			movementSystem->OnMove(olc::vf2d(0.0f, -1.0f) * fElapsedTime);
		}
		else if (GetKey(olc::Key::DOWN).bHeld)
		{
			movementSystem->OnMove(olc::vf2d(0.0f, 1.0f) * fElapsedTime);
		}

		if (GetKey(olc::Key::RIGHT).bHeld)
		{
			movementSystem->OnMove(olc::vf2d(1.0f, 0.0f) * fElapsedTime);
		}
		else if (GetKey(olc::Key::LEFT).bHeld)
		{
			movementSystem->OnMove(olc::vf2d(-1.0f, 0.0f) * fElapsedTime);
		}

		if (GetKey(olc::Key::SPACE).bPressed)
		{
			CreateBullet(collisionSystem->GetEntity(0));
		}

		bulletSystem->MoveBullet(fElapsedTime, this, collisionSystem);

		SpawnEnemy(fElapsedTime);
		aiSystem->Move(fElapsedTime, this, collisionSystem);
		aiSystem->Shoot(fElapsedTime);

		return true;
	}

	void CreateBullet(Entity owner)
	{
		auto& transform = g_Coordinator.GetComponent<Transform>(owner);
		auto& collision = g_Coordinator.GetComponent<Collision>(owner);

		olc::vf2d scale = olc::vf2d(0.1f, 0.1f);
		std::shared_ptr<olc::Decal> decal = std::make_shared<olc::Decal>(bulletSprite);

		Entity entity = g_Coordinator.CreateEntity();
		g_Coordinator.AddComponent(entity, Transform{ .position = transform.position + olc::vf2d(5.0f, -10.0f), .scale = scale });
		g_Coordinator.AddComponent(entity, Graphic{ .decal = decal, .tint = olc::WHITE });
		g_Coordinator.AddComponent(entity, Bullet{ .velocity = olc::vf2d(0.0f, -150.0f) });
		g_Coordinator.AddComponent(entity, Collision{ .radius = 2.0f,
													  .center = olc::vf2d(decal.get()->sprite->width * 0.5f,
																		  decal.get()->sprite->height * 0.5f) * scale });
	}

	void SpawnEnemy(float deltaTime)
	{
		spawnTimer += deltaTime;
		if (spawnTimer >= spawnInterval)
		{
			olc::vf2d scale = olc::vf2d(0.1f, 0.1f);
			std::shared_ptr<olc::Decal> enemyDecal = std::make_shared<olc::Decal>(enemySprite);

			int enemyCount = rand() % 10;
			if (enemyCount < 1)
			{
				enemyCount = 1;
			}
			for (size_t i = 0; i < enemyCount; i++)
			{
				Entity entity = g_Coordinator.CreateEntity();

				olc::vf2d randomPosition = olc::vf2d(rand() % (ScreenWidth() - (enemyDecal.get()->sprite->width)), -(rand() % (int)(ScreenHeight() - (enemyDecal.get()->sprite->width) * 0.5f)));

				g_Coordinator.AddComponent(entity, Transform{ .position = randomPosition, .scale = scale });
				g_Coordinator.AddComponent(entity, Graphic{ .decal = enemyDecal, .tint = olc::WHITE });
				g_Coordinator.AddComponent(entity, AI{ .velocity = olc::vf2d(0.0f, 50.0f), .shootInterval = 2.0f });
				g_Coordinator.AddComponent(entity, Collision{ .radius = 6.0f,
															  .center = olc::vf2d(enemyDecal.get()->sprite->width * 0.5f,
																				  enemyDecal.get()->sprite->height * 0.5f) * scale });
			}

			

			spawnTimer -= spawnInterval;
		}
	}

};

int main()
{
	SpaceShooter demo;
	if (demo.Construct(256, 240, 4, 4))
		demo.Start();

	return 0;
}