#include "Utilities.h"
#include "../CCL.h"
namespace UTIL
{
	GW::MATH::GVECTORF GetRandomVelocityVector()
	{
		GW::MATH::GVECTORF vel = {float((rand() % 20) - 10), 0.0f, float((rand() % 20) - 10)};
		if (vel.x <= 0.0f && vel.x > -1.0f)
			vel.x = -1.0f;
		else if (vel.x >= 0.0f && vel.x < 1.0f)
			vel.x = 1.0f;

		if (vel.z <= 0.0f && vel.z > -1.0f)
			vel.z = -1.0f;
		else if (vel.z >= 0.0f && vel.z < 1.0f)
			vel.z = 1.0f;

		GW::MATH::GVector::NormalizeF(vel, vel);

		return vel;
	}

	void CreateDynamicObjects(entt::registry& registry, std::string modelName, DRAW::MeshCollection& MeshCollection, GAME::Transform& Transform) {
		// Input validation and initial debug logging
		std::cout << "[DEBUG] CreateDynamicObjects - Starting creation for model: " << modelName << std::endl;

		if (modelName.empty()) {
			std::cerr << "[ERROR] CreateDynamicObjects - Model name is empty" << std::endl;
			return;
		}

		// Get model manager with error checking
		try {
			auto& modelManager = registry.ctx().get<DRAW::ModelManager>();
			std::cout << "[DEBUG] CreateDynamicObjects - Successfully retrieved ModelManager" << std::endl;

			// Verify model exists
			if (modelManager.models.find(modelName) == modelManager.models.end()) {
				std::cerr << "[ERROR] CreateDynamicObjects - Model '" << modelName << "' not found in ModelManager" << std::endl;
				return;
			}

			bool setTransform = false;

			// Log mesh count before copying
			std::cout << "[DEBUG] CreateDynamicObjects - Copying meshes for model: " << modelName
				<< ", Source mesh count: " << modelManager.models[modelName].meshes.size() << std::endl;

			std::vector<entt::entity> meshes = CopyRenderableEntities(registry, modelManager.models[modelName].meshes);
			std::cout << "[DEBUG] CreateDynamicObjects - Created " << meshes.size() << " mesh entities" << std::endl;

			// Track mesh processing
			int meshCounter = 0;
			for (auto var : meshes) {
				meshCounter++;
				std::cout << "[DEBUG] Processing mesh " << meshCounter << " of " << meshes.size() << std::endl;

				auto entity = registry.create();
				std::cout << "[DEBUG] Created new entity: " << static_cast<uint32_t>(entity) << std::endl;

				try {
					DRAW::GPUInstance gpuInstance = registry.get<DRAW::GPUInstance>(var);
					DRAW::GeometryData geometryData = registry.get<DRAW::GeometryData>(var);

					registry.emplace<DRAW::GPUInstance>(entity, gpuInstance);
					registry.emplace<DRAW::GeometryData>(entity, geometryData);

					if (!setTransform) {
						Transform.transform = gpuInstance.transform;
						setTransform = true;
						std::cout << "[DEBUG] Set initial transform from mesh " << meshCounter << std::endl;
					}

					MeshCollection.meshes.push_back(entity);

					// Log bounding box information
					GW::MATH::GOBBF boundingBox = modelManager.models[modelName].boundingBox;
					MeshCollection.boundingBox = boundingBox;
					std::cout << "[DEBUG] Added bounding box for mesh " << meshCounter << std::endl;

				}
				catch (const std::exception& e) {
					std::cerr << "[ERROR] Exception while processing mesh " << meshCounter
						<< ": " << e.what() << std::endl;
				}
			}

			// Final status report
			std::cout << "[DEBUG] CreateDynamicObjects - Completed successfully\n"
				<< "- Total meshes processed: " << meshCounter << "\n"
				<< "- MeshCollection size: " << MeshCollection.meshes.size() << "\n"
				<< "- Transform set: " << (setTransform ? "Yes" : "No") << std::endl;

		}
		catch (const std::exception& e) {
			std::cerr << "[ERROR] CreateDynamicObjects - Fatal error: " << e.what() << std::endl;
		}
	}


	std::vector<entt::entity> CopyRenderableEntities(entt::registry& registry, std::vector<entt::entity> entitiesToCopy) {
		std::vector<entt::entity> newEntities;
		for each (auto entity in entitiesToCopy)
		{
			auto newEntity = registry.create();
			registry.emplace<DRAW::DoNotRender>(newEntity);
			auto gpuInstance = registry.get<DRAW::GPUInstance>(entity);
			auto geometryData = registry.get<DRAW::GeometryData>(entity);

			DRAW::GPUInstance gpuInstanceCopy = DRAW::GPUInstance{ gpuInstance.transform, gpuInstance.matData };
			DRAW::GeometryData geometryDataCopy = DRAW::GeometryData{ geometryData.indexStart, geometryData.indexCount, geometryData.vertexStart };

			registry.emplace<DRAW::GPUInstance>(newEntity, gpuInstanceCopy);
			registry.emplace<DRAW::GeometryData>(newEntity, geometryDataCopy);

			newEntities.push_back(newEntity);
		}
		return newEntities;
	}
	void UpdateUILevel(entt::registry& registry, int level)
	{
		auto UIview = registry.view<GAME::UIComponents>();
		if (UIview.begin() != UIview.end()) {
			auto& front = *UIview.begin();
			auto& UIElements = registry.get<GAME::UIComponents>(front);
			UIElements.currentLevel = level;
		}
	}
	void UpdateUILives(entt::registry& registry, int newLives)
	{
		auto UIview = registry.view<GAME::UIComponents>();
		if (UIview.begin() != UIview.end()) {
			auto& front = *UIview.begin();
			auto& UIElements = registry.get<GAME::UIComponents>(front);
			UIElements.lives = newLives;
		}
	}
	void UpdateUIActiveScore(entt::registry& registry, int newScore)
	{
		auto UIview = registry.view<GAME::UIComponents>();
		if (UIview.begin() != UIview.end()) {
			auto& front = *UIview.begin();
			auto& UIElements = registry.get<GAME::UIComponents>(front);
			UIElements.currScore += newScore;
			std::cout << UIElements.currScore;
		}
	}
	void UpdateUIHighScore(entt::registry& registry, int newScore)
	{
		auto UIview = registry.view<GAME::UIComponents>();
		if (UIview.begin() != UIview.end()) {
			auto& front = *UIview.begin();
			auto& UIElements = registry.get<GAME::UIComponents>(front);
			UIElements.highScore = newScore;
		}
	}

	void CheckPausePressed(entt::registry& registry) {
		auto& input = registry.ctx().get<UTIL::Input>();
		auto& events = registry.ctx().get<GW::CORE::GEventCache>();
		GW::GEvent e;
		while (+events.Pop(e)) {
			GW::INPUT::GBufferedInput::Events g;
			GW::INPUT::GBufferedInput::EVENT_DATA d;
			if (+e.Read(g, d)) {
				if (g == GW::INPUT::GBufferedInput::Events::KEYPRESSED && d.data == G_KEY_P) {
					auto gameManager = registry.view<GAME::GameManager>().front();
					if (registry.all_of<GAME::Paused>(gameManager) == false) {
						registry.emplace<GAME::Paused>(gameManager);
						std::cout << "Game Paused" << std::endl;
					}
					else {
						registry.remove<GAME::Paused>(gameManager);
						std::cout << "Game Unpaused" << std::endl;
					}
				}
			}

		}
		/*float p = 0.0f;
		if (p > 0.0f ) {
			auto gameManager = registry.view<GAME::GameManager>().front();
			if (registry.all_of<GAME::Paused>(gameManager) == false) {
				registry.emplace<GAME::Paused>(gameManager);
				std::cout << "Game Paused" << std::endl;
			}
			else {
				registry.remove<GAME::Paused>(gameManager);
				std::cout << "Game Unpaused" << std::endl;
			}
		}*/
	}
} // namespace UTIL