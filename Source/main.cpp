// main entry point for the application
// enables components to define their behaviors locally in an .hpp file
#include "CCL.h"
#include "UTIL/Utilities.h"
// include all components, tags, and systems used by this program
#include "DRAW/DrawComponents.h"
#include "GAME/GameComponents.h"
#include "APP/Window.hpp"


// Local routines for specific application behavior
void GraphicsBehavior(entt::registry& registry);
void MainMenuBehavior(entt::registry& registry);
void GameplayBehavior(entt::registry& registry);
void MainLoopBehavior(entt::registry& registry);
void InitializeUI(entt::registry& registry);

// Architecture is based on components/entities pushing updates to other components/entities (via "patch" function)
int main()
{
	// All components, tags, and systems are stored in a single registry
	entt::registry registry;

	// initialize the ECS Component Logic
	CCL::InitializeComponentLogic(registry);

	// Seed the rand
	unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
	srand(time);

	registry.ctx().emplace<UTIL::Config>();

	GraphicsBehavior(registry); // create windows, surfaces, and renderers

	InitializeUI(registry);

	MainMenuBehavior(registry);

	MainLoopBehavior(registry); // update windows and input

	// clear all entities and components from the registry
	// invokes on_destroy() for all components that have it
	// registry will still be intact while this is happening
	registry.clear();

	return 0; // now destructors will be called for all components
}

// This function will be called by the main loop to update the graphics
// It will be responsible for loading the Level, creating the VulkanRenderer, and all VulkanInstances
void GraphicsBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

	// Add an entity to handle graphics
	auto display = registry.create();

	registry.ctx().emplace<entt::entity>(display);

	// Emplace and initialize Window component
	int windowWidth = (*config).at("Window").at("width").as<int>();
	int windowHeight = (*config).at("Window").at("height").as<int>();
	int startX = (*config).at("Window").at("xstart").as<int>();
	int startY = (*config).at("Window").at("ystart").as<int>();

	registry.emplace<APP::Window>(display,
		APP::Window{ startX, startY, windowWidth, windowHeight, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED, "Blue Team - Shooty McRockFace" });

	// Create input system
	auto& input = registry.ctx().emplace<UTIL::Input>();
	auto& window = registry.get<GW::SYSTEM::GWindow>(display);
	input.bufferedInput.Create(window);
	input.immediateInput.Create(window);
	input.gamePads.Create();
	auto& pressEvents = registry.ctx().emplace<GW::CORE::GEventCache>();
	pressEvents.Create(32);
	input.bufferedInput.Register(pressEvents);
	input.gamePads.Register(pressEvents);

	// Create Vulkan Renderer (Setting up, NO level yet)
	std::string vertShader = (*config).at("Shaders").at("vertex").as<std::string>();
	std::string pixelShader = (*config).at("Shaders").at("pixel").as<std::string>();
	registry.emplace<DRAW::VulkanRendererInitialization>(display,
		DRAW::VulkanRendererInitialization
		{
			vertShader, pixelShader,
			{{0.2f, 0.2f, 0.25f, 1} }, { 1.0f, 0u }, 75.f, 0.1f, 100.0f });

	registry.emplace<DRAW::VulkanRenderer>(display);

	// Register for Vulkan clean up
	GW::CORE::GEventResponder shutdown;
	shutdown.Create([&](const GW::GEvent& e) {
		GW::GRAPHICS::GVulkanSurface::Events event;
		GW::GRAPHICS::GVulkanSurface::EVENT_DATA data;
		if (+e.Read(event, data) && event == GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES) 
		{
			registry.clear<DRAW::VulkanRenderer>();
		}
		});
	registry.get<DRAW::VulkanRenderer>(display).vlkSurface.Register(shutdown);
	registry.emplace<GW::CORE::GEventResponder>(display, shutdown.Relinquish());

	// Create camera
	GW::MATH::GMATRIXF initialCamera;
	GW::MATH::GVECTORF translate = { 0.0f,  45.0f, -5.0f };
	GW::MATH::GVECTORF lookat = { 0.0f, 0.0f, 0.0f };
	GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f };
	GW::MATH::GMatrix::TranslateGlobalF(initialCamera, translate, initialCamera);
	GW::MATH::GMatrix::LookAtLHF(translate, lookat, up, initialCamera);
	GW::MATH::GMatrix::InverseF(initialCamera, initialCamera);
	registry.emplace<DRAW::Camera>(display, DRAW::Camera{ initialCamera });
}


void MainMenuBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
	entt::entity display = registry.ctx().get<entt::entity>();

	std::string menuPath = (*config).at("Menu").at("menuFile").as<std::string>();
	std::string modelPath = (*config).at("Menu").at("modelPath").as<std::string>();
	std::string music = (*config).at("Menu").at("music").as<std::string>();

	auto& gAudio = registry.ctx().emplace<GW::AUDIO::GAudio>();
	gAudio.Create();
	gAudio.SetMasterVolume(0.1f);

	auto& gMusic = registry.ctx().emplace<GW::AUDIO::GMusic>();
	gMusic.Create(music.c_str(), gAudio);
	gMusic.Play(true);

	if (!registry.all_of<DRAW::CPULevel>(display)) 
	{
		registry.emplace<DRAW::CPULevel>(display, DRAW::CPULevel{ menuPath, modelPath });
	}

	if (!registry.all_of<DRAW::GPULevel>(display)) 
	{
		registry.emplace<DRAW::GPULevel>(display);
	}
}

void LoadLevelOne(entt::registry& registry)
{
	std::cout << "Starting LoadLevelOne\n";
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

	if (!registry.ctx().contains<entt::entity>()) 
	{
		return;
	}

	entt::entity display = registry.ctx().get<entt::entity>();

	if (!registry.valid(display)) 
	{
		return;
	}

	if (registry.all_of<DRAW::GPULevel>(display)) 
	{
		registry.remove<DRAW::GPULevel>(display);
	}

	if (registry.all_of<DRAW::CPULevel>(display)) 
	{
		registry.remove<DRAW::CPULevel>(display);
	}

	if (registry.all_of<DRAW::VulkanRenderer>(display)) 
	{
		registry.patch<DRAW::VulkanRenderer>(display);
	}
	else 
	{
		return;
	}

	std::string levelPath = (*config).at("Level1").at("levelFile").as<std::string>();
	std::string modelPath = (*config).at("Level1").at("modelPath").as<std::string>();

	registry.emplace<DRAW::CPULevel>(display, DRAW::CPULevel{ levelPath, modelPath });

	std::string music = (*config).at("Level1").at("music").as<std::string>();

	auto& gAudio = registry.ctx().emplace<GW::AUDIO::GAudio>();
	gAudio.Create();
	gAudio.SetMasterVolume(0.1f);

	auto& gMusic = registry.ctx().emplace<GW::AUDIO::GMusic>();
	gMusic.Create(music.c_str(), gAudio);
	gMusic.Play(true);

	if (!registry.all_of<DRAW::CPULevel>(display)) 
	{
		return;
	}

	if (registry.all_of<DRAW::GPULevel>(display)) 
	{
		registry.replace<DRAW::GPULevel>(display);
	}
	else {
		registry.emplace<DRAW::GPULevel>(display);
	}

	if (!registry.all_of<DRAW::GPULevel>(display)) 
	{
		return;
	}

	if (registry.all_of<DRAW::VulkanRenderer>(display)) 
	{
		registry.patch<DRAW::VulkanRenderer>(display);
	}
	else
	{
		return;
	}
	std::cout << "Initializing UI\n";

	UTIL::UpdateUILevel(registry, 1);

	std::cout << "Ending LoadLevelOne\n";
}

void InitializeUI(entt::registry& registry)
{
	// emplace the UIComponent to help track UI numbers
	auto UI = registry.create();
	registry.emplace<GAME::UIComponents>(UI, GAME::UIComponents{ 0, 0, 0, 0 });
}

//function to create dynamic objects in GamePlayBehavior
// This function will be called by the main loop to update the gameplay
// It will be responsible for updating the VulkanInstances and any other gameplay components
void GameplayBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

	auto player = registry.create();
	auto enemy1 = registry.create();
	auto enemy2 = registry.create();
	auto enemy3_ufo = registry.create();

	registry.emplace<GAME::Player>(player);
	auto& playerMeshes = registry.emplace<DRAW::MeshCollection>(player);
	auto& playerTransform = registry.emplace<GAME::Transform>(player);
	registry.emplace<GAME::Collidable>(player);
	registry.emplace<GAME::Health>(player, GAME::Health{ config.get()->at("Player").at("hitpoints").as<int>() });
	UTIL::UpdateUILives(registry, config.get()->at("Player").at("hitpoints").as<int>());

	if (registry.ctx().contains<GW::AUDIO::GAudio>())
	{
		auto& gAudio = registry.ctx().get<GW::AUDIO::GAudio>();

		std::string shotSound = (*config).at("Player").at("shotSound").as<std::string>();
		auto& pewPew = registry.emplace<GAME::PewPew>(player).pewPew;
		pewPew.Create(shotSound.c_str(), gAudio);
	}

	registry.emplace<GAME::Enemy>(enemy1);
	registry.emplace<GAME::Collidable>(enemy1);
	auto& enemyMeshes = registry.emplace<DRAW::MeshCollection>(enemy1);
	auto& enemyTransform = registry.emplace<GAME::Transform>(enemy1);
	auto& enemyVelocity = registry.emplace<GAME::Velocity>(enemy1, GAME::Velocity{ UTIL::GetRandomVelocityVector() });
	registry.emplace<GAME::Health>(enemy1, GAME::Health{ config.get()->at("Enemy1").at("hitpoints").as<int>() });
	registry.emplace<GAME::Shatters>(enemy1, GAME::Shatters{ config.get()->at("Enemy1").at("initialShatterCount").as<int>() });
	GW::MATH::GVector::ScaleF(enemyVelocity.velocity, config.get()->at("Enemy1").at("speed").as<float>(), enemyVelocity.velocity);

	registry.emplace<GAME::Enemy>(enemy2);
	registry.emplace<GAME::Collidable>(enemy2);
	auto& enemy2Meshes = registry.emplace<DRAW::MeshCollection>(enemy2);
	auto& enemy2Transform = registry.emplace<GAME::Transform>(enemy2);
	auto& enemy2Velocity = registry.emplace<GAME::Velocity>(enemy2, GAME::Velocity{ UTIL::GetRandomVelocityVector() });
	registry.emplace<GAME::Health>(enemy2, GAME::Health{ config.get()->at("Enemy2").at("hitpoints").as<int>() });
	registry.emplace<GAME::Shatters>(enemy2, GAME::Shatters{ config.get()->at("Enemy2").at("initialShatterCount").as<int>() });
	GW::MATH::GVector::ScaleF(enemy2Velocity.velocity, config.get()->at("Enemy2").at("speed").as<float>(), enemy2Velocity.velocity);


	registry.emplace <GAME::Enemy>(enemy3_ufo);
	registry.emplace<GAME::Collidable>(enemy3_ufo);
	auto& enemy3Meshes = registry.emplace<DRAW::MeshCollection>(enemy3_ufo);
	auto& enemy3Transform = registry.emplace<GAME::Transform>(enemy3_ufo);
	auto& enemy3Velocity = registry.emplace<GAME::Velocity>(enemy3_ufo, GAME::Velocity{ UTIL::GetRandomVelocityVector() });
	registry.emplace<GAME::Health>(enemy3_ufo, GAME::Health{ config.get()->at("Enemy3").at("hitpoints").as<int>() });
	GW::MATH::GVector::ScaleF(enemy3Velocity.velocity, config.get()->at("Enemy3").at("speed").as<float>(), enemy3Velocity.velocity);



	std::cout << "Loading Player Model\n";
	auto playerModel = config.get()->at("Player").at("model").as<std::string>();
	UTIL::CreateDynamicObjects(registry, playerModel, playerMeshes, playerTransform);

	std::cout << "Loading Enemy1 Model\n";
	auto enemyModel = config.get()->at("Enemy1").at("model").as<std::string>();
	UTIL::CreateDynamicObjects(registry, enemyModel, enemyMeshes, enemyTransform);

	std::cout << "Loading Enemy2 Model\n";
	auto enemy2Model = config.get()->at("Enemy2").at("model").as<std::string>();
	UTIL::CreateDynamicObjects(registry, enemy2Model, enemy2Meshes, enemy2Transform);

	std::cout << "Loading Enemy3 Model\n";
	auto enemy3Model = config.get()->at("Enemy3").at("model").as<std::string>();
	UTIL::CreateDynamicObjects(registry, enemy3Model, enemy3Meshes, enemy3Transform);
	// Create Game Manager
	auto gameManager = registry.create();
	registry.emplace<GAME::GameManager>(gameManager);
}



// This function will be called by the main loop to update the main loop
// It will be responsible for updating any created windows and handling any input
void MainLoopBehavior(entt::registry& registry)
{
	if (!registry.ctx().contains<entt::entity>()) 
	{
		return;
	}

	entt::entity display = registry.ctx().get<entt::entity>();

	int closedCount;
	auto winView = registry.view<APP::Window>();
	auto& deltaTime = registry.ctx().emplace<UTIL::DeltaTime>().dtSec;
	auto& gameState = registry.ctx().emplace<GAME::GameState>(GAME::GameState::MAIN_MENU);
	auto& input = registry.ctx().get<UTIL::Input>();

	do 
	{
		static auto start = std::chrono::steady_clock::now();
		double elapsed = std::chrono::duration<double>(
			std::chrono::steady_clock::now() - start).count();
		start = std::chrono::steady_clock::now();
		if (elapsed > 1.0 / 30.0) elapsed = 1.0 / 30.0;
		deltaTime = elapsed;

		if (gameState == GAME::GameState::MAIN_MENU) 
		{
			float enterState = 0.0f;
			if (input.immediateInput.GetState(G_KEY_ENTER, enterState) == GW::GReturn::SUCCESS)
			{
				if (enterState > 0.0f) 
				{
					gameState = GAME::GameState::GAMEPLAY;

					if (registry.ctx().contains<GW::AUDIO::GMusic>())
					{
						auto& gMusic = registry.ctx().get<GW::AUDIO::GMusic>();
						gMusic.Stop();
						registry.ctx().erase<GW::AUDIO::GMusic>();
					}

					if (registry.all_of<DRAW::CPULevel>(display)) 
					{
						registry.remove<DRAW::CPULevel>(display);
					}

					LoadLevelOne(registry);

					if (registry.all_of<DRAW::VulkanRenderer>(display)) 
					{
						registry.patch<DRAW::VulkanRenderer>(display);
					}

					GameplayBehavior(registry);
				}
			}
		}
		
		if (gameState == GAME::GameState::GAMEOVER)
		{
			float enterState = 0.0f;
			if (input.immediateInput.GetState(G_KEY_ENTER, enterState) == GW::GReturn::SUCCESS)
			{
				if (enterState > 0.0f)
				{
					// Stop Game Over Music. Uncomment when/if Game over music is added
					if (registry.ctx().contains<GW::AUDIO::GMusic>())
					{
						auto& gMusic = registry.ctx().get<GW::AUDIO::GMusic>();
						gMusic.Stop();
						registry.ctx().erase<GW::AUDIO::GMusic>();
					}

					// Reset Game State back to Main Menu
					gameState = GAME::GameState::MAIN_MENU;

					// Unload Level
					if (registry.all_of<DRAW::GPULevel>(display))
					{
						registry.remove<DRAW::GPULevel>(display);
					}

					if (registry.all_of<DRAW::CPULevel>(display))
					{
						registry.remove<DRAW::CPULevel>(display);
					}

					// Remove all gameplay-related entities
					registry.clear<GAME::Player, GAME::Enemy, GAME::Projectile, GAME::GameManager, GAME::Collidable, GAME::Shatters, GAME::Health, GAME::Velocity>();

					// Ensure VulkanRenderer updates after removing Level 1
					if (registry.all_of<DRAW::VulkanRenderer>(display))
					{
						registry.patch<DRAW::VulkanRenderer>(display);
					}

					//Load the Main Menu
					MainMenuBehavior(registry);
				}
			}
		}

		// Game updates
		if (gameState == GAME::GameState::GAMEPLAY) 
		{
			auto gameManager = registry.view<GAME::GameManager>().front();
			
			if (!registry.all_of<GAME::GameOver>(gameManager)  ) 
			{
				UTIL::CheckPausePressed(registry);
				if(!registry.all_of<GAME::Paused>(gameManager))
				registry.patch<GAME::GameManager>(gameManager);
			}
		}

		// Window updates
		closedCount = 0;
		for (auto entity : winView) 
		{
			if (registry.any_of<APP::WindowClosed>(entity))
				++closedCount;
			else
				registry.patch<APP::Window>(entity);
		}
	} 
	while (winView.size() != closedCount);
}