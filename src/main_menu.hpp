#pragma once

#include <imgui.h>

#include "./threads/clock.hpp"
#include "./Programs.hpp"

void main_menu(const sf::RenderWindow &window, const std::shared_ptr<Chip8> chip8, std::unique_ptr<std::thread> &clock_thread, std::shared_ptr<Debugger> &debugger)
{
    static Programs programs("../prog_list.txt");
    static Program *selected_program = &programs.programs[0];

    ImGui::Begin("Main Menu", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowSize(ImVec2(450, 100));
    ImGui::SetWindowPos(ImVec2((Chip8::SCREEN_WIDTH * Chip8::PIXEL_SIZE - 450) / 2, (Chip8::SCREEN_HEIGHT * Chip8::PIXEL_SIZE - 100) / 2));

    // Display the dropdown of programs
    if (ImGui::BeginCombo("Programs", selected_program->name.c_str()))
    {
        for (Program &program : programs.programs)
        {
            if (ImGui::Selectable(program.name.c_str(), &program == selected_program))
            {
                selected_program = &program;
            }

            if (&program == selected_program)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Enable/Disable the debugger
    if (ImGui::Button(debugger ? "Disable Debugger" : "Enable Debugger"))
    {
        if (debugger)
            debugger = nullptr;
        else
            debugger = std::make_shared<Debugger>();
    }

    // Load the program and start the CPU
    if (ImGui::Button("Go"))
    {
        chip8->load_program(selected_program);
        if (debugger)
            chip8->attach_debugger(debugger);
        clock_thread = create_clock_thread(window, chip8);
    }

    ImGui::End();
}