#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
	///*** Tags ***///
	struct Player {
	};
	struct Enemy {
	};
	struct Projectile {
	};
	struct GameManager {
	};
	struct Collidable {
	};
	struct Obstacle {
	};
	struct ToDestroy {
	};
	struct GameOver {
	};
	struct Paused{
	};

	///*** Components ***///
	struct Transform{
		GW::MATH::GMATRIXF transform;
	};
	
	struct FiringState {
		float fireCoolDown;
	};
	struct Velocity {
		GW::MATH::GVECTORF velocity;
	};
	struct Health {
		int health;
	};
	struct Shatters {
		int shatterCount;
	};
	struct Invulnerable {
		float invulnerableTime;
	};
	struct Rotation 
	{
		float angle = 0.0f;
		float angularVelocity = 0.0f;
	};
	struct Physics 
	{
		GW::MATH::GVECTORF velocity = { 0.0f, 0.0f, 0.0f };
		float thrust = 3.0f;
		float drag = 0.98f;
		float maxSpeed = 5.0f;
	};
	struct UIComponents {
		int lives;
		int currScore;
		int highScore;
		int currentLevel;
	};
	struct PewPew {
		GW::AUDIO::GSound pewPew;
	};

	//*** Game State ***//
	enum class GameState 
	{
		MAIN_MENU,
		GAMEPLAY,
		GAMEOVER
	};


}// namespace GAME
#endif // !GAME_COMPONENTS_H_