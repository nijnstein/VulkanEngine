#pragma once
 
class ConsoleWindow : public ui::Window
{
	const float BUTTON_HEIGHT = 22;
	const float COLLAPSED_HEIGHT = 38;
	const float OPEN_HEIGHT = 400;
	const float OPEN_WIDTH = 600; 	 // needs additonal window for that.. 

	float height = COLLAPSED_HEIGHT;
	bool showOutput = false;

	std::vector<std::string> previousCommands{};
	std::string consoleOutput{};
	std::string currentCommand{};

public:
	ConsoleWindow()
	{
		currentCommand.resize(1024);
		consoleOutput.reserve(1024);

		init("Console",
			{ 0, 0 },
			{ 10, 10 },
			false,
			false,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
	}

	void onResize(VulkanEngine* engine) override
	{
		position = { 0, engine->swapChainExtent.height - height };
		extents = { engine->swapChainExtent.width, height };
	}

	void onRender(VulkanEngine* engine, float deltaTime) override
	{
		if (showOutput)
		{
			ImGui::InputTextMultiline("##consoleOutput", consoleOutput.data(), consoleOutput.size(), { extents.x, height - 40 }, ImGuiInputTextFlags_ReadOnly);
		}

		bool toggleOutput = ImGui::Button(showOutput ? "Hide Log" : "Show Log", { 100, BUTTON_HEIGHT });
		
		ImGui::SameLine();
		ImGui::PushItemWidth(extents.x - 240);
		bool execute = ImGui::InputText
		(
			"##Command",
			currentCommand.data(), currentCommand.size(),
			ImGuiInputTextFlags_None | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll
		);

		ImGui::SameLine();
		execute |= ImGui::Button("Execute", { 100, BUTTON_HEIGHT });

		if (execute)
		{
			previousCommands.push_back(currentCommand.c_str());
			consoleOutput = consoleOutput + "CMD: " + currentCommand + "\n";
			currentCommand.clear();
			currentCommand.resize(1024);
			showOutput = false; 
			toggleOutput = true;
		}

		if(toggleOutput)
		{
			showOutput = !showOutput;
			if (showOutput)
			{
				height = OPEN_HEIGHT;
			}
			else
			{
				height = COLLAPSED_HEIGHT;
			}
			onResize(engine);

			ImGui::SetWindowPos({ position.x, position.y }, ImGuiCond_Always);
			ImGui::SetWindowSize({ extents.x, extents.y }, ImGuiCond_Always);
		}
	}
};
