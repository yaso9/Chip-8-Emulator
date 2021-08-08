#pragma once

#include <algorithm>
#include <iomanip>
#include <optional>
#include <sstream>
#include <unordered_set>

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

#include "./get_bits.hpp"
#include "./Registers.hpp"

class Debugger
{
private:
    std::shared_ptr<std::array<byte, 0x1000>> memory;
    std::shared_ptr<Registers> registers;

    MemoryEditor memory_editor;

    // Set of addresses with breakpoints
    std::unordered_set<addr_t> breakpoints{};

    // If set when a clock starts, a breakpoint will be triggered
    bool break_next = false;

    // The register currently being edited, nullptr if none are being edited
    void *editing_reg;

    // These are used to handle pausing program execution for breakpoints
    std::mutex break_mtx;
    std::condition_variable break_cv;
    bool broken;

    template <uint8_t Digits>
    constexpr const std::string get_register_format() const
    {
        std::ostringstream stream;
        stream << "%0" << static_cast<int>(Digits) << "X";
        std::string str = stream.str();
        return str;
    }

    template <uint8_t Digits, typename Type>
    void draw_hex_input(const char *label, Type *data) const
    {
        static const std::string format = get_register_format<Digits>();

        char buf[Digits + 1];
        snprintf(buf, Digits + 1, format.c_str(), *data);

        if (ImGui::InputText(label, buf, Digits + 1, ImGuiInputTextFlags_CharsHexadecimal))
        {
            sscanf(buf, format.c_str(), data);
        }
    }

    template <uint8_t Digits, typename Type>
    void draw_register(std::string name, Type *reg)
    {
        static const std::string format = get_register_format<Digits>();

        ImGui::TableNextColumn();
        ImGui::Text("%s: ", name.c_str());
        ImGui::SameLine();
        if (editing_reg != reg)
            ImGui::Text(format.c_str(), (int)*reg);
        else
        {
            draw_hex_input<Digits, Type>(("##" + name).c_str(), static_cast<Type *>(editing_reg));
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            if (ImGui::IsItemHovered())
                editing_reg = reg;
            else if (editing_reg == reg)
                editing_reg = nullptr;
    }

    void draw_registers()
    {
        if (ImGui::BeginTable("registers", 5))
        {
            // Display the general registers
            for (uint8_t i = 0; i <= 0xF; i++)
            {
                std::ostringstream stream;
                stream << 'R' << std::hex << std::uppercase << static_cast<int>(i);
                draw_register<2>(stream.str().c_str(), &registers->general_regs[i]);
            }

            draw_register<3>("I", &registers->addr_reg);
            draw_register<2>("DT", &registers->delay_reg);
            draw_register<2>("ST", &registers->sound_reg);
            draw_register<3>("PC", &registers->pc_reg);

            ImGui::EndTable();
        }
    }

    class Instruction
    {
    public:
        class Argument
        {
        public:
            enum class Type
            {
                GeneralRegister,
                AddrRegister,
                DelayTimerRegister,
                SoundTimerRegister,
                Key,
                Font,
                BCD,
                Address,
                Byte,
                Nibble
            };

            const Type type;
            const uint16_t value;
            const bool dereference;

            constexpr Argument(Type type, uint16_t value = 0, bool dereference = false) : type(type), value(value), dereference(dereference) {}

            operator std::string() const
            {
                std::stringstream stream;
                stream << std::hex << std::uppercase << std::setfill('0');

                if (dereference)
                    stream
                        << "[";

                switch (type)
                {
                case Type::GeneralRegister:
                    stream << "V" << value;
                    break;
                case Type::AddrRegister:
                    stream << "I";
                    break;
                case Type::DelayTimerRegister:
                    stream << "DT";
                    break;
                case Type::SoundTimerRegister:
                    stream << "ST";
                    break;
                case Type::Key:
                    stream << "K";
                    break;
                case Type::Font:
                    stream << "F";
                    break;
                case Type::BCD:
                    stream << "B";
                    break;
                case Type::Address:
                    stream << std::setw(3) << value;
                    break;
                case Type::Byte:
                    stream << std::setw(2) << value;
                    break;
                case Type::Nibble:
                    stream << std::setw(1) << value;
                    break;
                }

                if (dereference)
                    stream << "]";

                return stream.str();
            }
        };

        const std::string mnemonic;
        const std::optional<Argument> arg1;
        const std::optional<Argument> arg2;
        const std::optional<Argument> arg3;

        Instruction(std::string mnemonic, std::optional<Argument> arg1 = std::nullopt, std::optional<Argument> arg2 = std::nullopt, std::optional<Argument> arg3 = std::nullopt) : mnemonic(mnemonic), arg1(arg1), arg2(arg2), arg3(arg3) {}

        operator std::string() const
        {
            std::stringstream stream;

            stream << mnemonic;

            if (arg1)
                stream << " " << static_cast<std::string>(*arg1);

            if (arg2)
                stream << ", " << static_cast<std::string>(*arg2);

            if (arg3)
                stream << ", " << static_cast<std::string>(*arg3);

            return stream.str();
        }
    };

    std::optional<Instruction> disassemble_instruction(inst_t instruction)
    {
        // nnn - A 12-bit value, the lowest 12 bits of the instruction
        const addr_t nnn = get_bits(instruction, 0, 12);
        // n - A 4-bit value, the lowest 4 bits of the instruction
        const uint8_t n = get_bits(instruction, 0, 4);
        // x - A 4-bit value, the lower 4 bits of the high byte of the instruction
        const uint8_t x = get_bits(instruction, 8, 4);
        // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
        const uint8_t y = get_bits(instruction, 4, 4);
        // kk - An 8-bit value, the lowest 8 bits of the instruction
        const uint8_t kk = get_bits(instruction, 0, 8);

        // Switch on the most significant nibble of the instruction
        switch (get_bits(instruction, 12, 4))
        {
        case 0x0:
            switch (nnn)
            {
            case 0x0E0:
                // 00E0 - CLS
                return Instruction("CLS");
                break;
            case 0x0EE:
                // 00EE - RET
                return Instruction("RET");
                break;
            }
            break;
        case 0x1:
            // 1nnn - JP addr
            return Instruction("JP", Instruction::Argument(Instruction::Argument::Type::Address, nnn));
            break;
        case 0x2:
            // 2nnn - CALL addr
            return Instruction("CALL", Instruction::Argument(Instruction::Argument::Type::Address, nnn));
            break;
        case 0x3:
            // 3xkk - SE Vx, byte
            return Instruction("SE", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                               Instruction::Argument(Instruction::Argument::Type::Byte, kk));
            break;
        case 0x4:
            // 4xkk - SNE Vx, byte
            return Instruction("SNE", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                               Instruction::Argument(Instruction::Argument::Type::Byte, kk));
            break;
        case 0x5:
            // 5xy0 - SE Vx, Vy
            return Instruction("SE", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                               Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
            break;
        case 0x6:
            // 6xkk - LD Vx, byte
            return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                               Instruction::Argument(Instruction::Argument::Type::Byte, kk));
            break;
        case 0x7:
            // 7xkk - ADD Vx, byte
            return Instruction("ADD", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                               Instruction::Argument(Instruction::Argument::Type::Byte, kk));
            break;
        case 0x8:
            switch (n)
            {
            case 0x0:
                // 8xy0 - LD Vx, Vy
                return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
                break;
            case 0x1:
                // 8xy1 - OR Vx, Vy
                return Instruction("OR", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
                break;
            case 0x2:
                // 8xy2 - AND Vx, Vy
                return Instruction("AND", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
                break;
            case 0x3:
                // 8xy3 - XOR Vx, Vy
                return Instruction("XOR", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
                break;
            case 0x4:
                // 8xy4 - ADD Vx, Vy
                return Instruction("ADD", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
                break;
            case 0x5:
                // 8xy5 - SUB Vx, Vy
                return Instruction("SUB", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
                break;
            case 0x6:
                // 8xy6 - SHR Vx {, Vy}
                return Instruction("SHR", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
                break;
            case 0x7:
                // 8xy7 - SUBN Vx, Vy
                return Instruction("SUBN", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
                break;
            case 0xE:
                // 8xyE - SHL Vx {, Vy}
                return Instruction("SHL", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
                break;
            }
            break;
        case 0x9:
            // 9xy0 - SNE Vx, Vy
            return Instruction("SNE", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                               Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y));
            break;
        case 0xA:
            // Annn - LD I, addr
            return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::AddrRegister),
                               Instruction::Argument(Instruction::Argument::Type::Address, nnn));
            break;
        case 0xB:
            // Bnnn - JP V0, addr
            return Instruction("JP", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, 0),
                               Instruction::Argument(Instruction::Argument::Type::Address, nnn));
            break;
        case 0xC:
            // Cxkk - RND Vx, byte
            return Instruction("RND", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                               Instruction::Argument(Instruction::Argument::Type::Byte, kk));
            break;
        case 0xD:
            // Dxyn - DRW Vx, Vy, nibble
            return Instruction("DRW", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                               Instruction::Argument(Instruction::Argument::Type::GeneralRegister, y),
                               Instruction::Argument(Instruction::Argument::Type::Nibble, n));
            break;
        case 0xE:
            switch (kk)
            {
            case 0x9E:
                // Ex9E - SKP Vx
                return Instruction("SKP", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x));
                break;
            case 0xA1:
                // ExA1 - SKNP Vx
                return Instruction("SKNP", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x));
                break;
            }
            break;
        case 0xF:
            switch (kk)
            {
            case 0x07:
                // Fx07 - LD Vx, DT
                return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::DelayTimerRegister));
                break;
            case 0x0A:
                // Fx0A - LD Vx, K
                return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::Key));
                break;
            case 0x15:
                // Fx15 - LD DT, Vx
                return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::DelayTimerRegister),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x));
                break;
            case 0x18:
                // Fx18 - LD ST, Vx
                return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::SoundTimerRegister),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x));
                break;
            case 0x1E:
                // Fx1E - ADD I, Vx
                return Instruction("ADD", Instruction::Argument(Instruction::Argument::Type::AddrRegister),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x));
                break;
            case 0x29:
                // Fx29 - LD F, Vx
                return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::Font),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x));
                break;
            case 0x33:
                // Fx33 - LD B, Vx
                return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::BCD),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x));
                break;
            case 0x55:
                // Fx55 - LD [I], Vx
                return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::AddrRegister, 0, true),
                                   Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x));
                break;
            case 0x65:
                // Fx65 - LD Vx, [I]
                return Instruction("LD", Instruction::Argument(Instruction::Argument::Type::GeneralRegister, x),
                                   Instruction::Argument(Instruction::Argument::Type::AddrRegister, 0, true));
                break;
            }
            break;
        }

        return std::nullopt;
    }

    void draw_disassembly()
    {
        if (ImGui::BeginTable("disassembly", 3, ImGuiTableFlags_ScrollY, ImVec2(0, 300)))
        {
            ImGuiListClipper clipper;
            clipper.Begin(memory->size() / 2);
            while (clipper.Step())
            {
                for (addr_t addr = clipper.DisplayStart * 2; addr < clipper.DisplayEnd * 2; addr += 2)
                {
                    std::optional<Instruction> instruction = disassemble_instruction((*memory)[addr] << 8 | (*memory)[addr + 1]);

                    ImGui::TableNextColumn();

                    bool breakpoint = breakpoints.count(addr);
                    if (breakpoint)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.27, 0, 1));

                    ImGui::Bullet();

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
                        if (!breakpoint)
                            breakpoints.insert(addr);
                        else
                            breakpoints.erase(addr);

                    if (breakpoint)
                        ImGui::PopStyleColor();

                    bool current_instruction = registers->pc_reg == addr;
                    if (current_instruction)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.27, 0, 1));

                    ImGui::TableNextColumn();
                    ImGui::Text("%03X", addr);

                    ImGui::TableNextColumn();
                    if (instruction)
                        ImGui::Text("%s", static_cast<std::string>(*instruction).c_str());
                    else
                        ImGui::Text("????");

                    if (current_instruction)
                        ImGui::PopStyleColor();
                }
            }
            ImGui::EndTable();
        }
    }

    // Handle drawing buttons for debugging operations, like step instruction, continue, etc.
    void draw_operations()
    {
        if (ImGui::Button("Continue"))
            continue_exec();

        ImGui::SameLine();

        if (ImGui::Button("Step Instruction"))
            step_instruction();
    }

    void continue_exec()
    {
        break_cv.notify_all();
    }

    void step_instruction()
    {
        break_next = true;
        continue_exec();
    }

public:
    void attach(std::shared_ptr<std::array<byte, 0x1000>> memory, std::shared_ptr<Registers> registers, bool break_next)
    {
        this->memory = memory;
        this->registers = registers;
        this->break_next = break_next;
    }

    void on_clock()
    {
        if (break_next || breakpoints.count(registers->pc_reg))
        {
            broken = true;

            break_next = false;

            std::unique_lock<std::mutex> lock(break_mtx);
            break_cv.wait(lock);
        }

        broken = false;
    }

    void draw_debugger()
    {
        ImGui::Begin("Debugger");

        if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen))
            draw_registers();

        if (ImGui::CollapsingHeader("Disassembler", ImGuiTreeNodeFlags_DefaultOpen))
        {
            draw_operations();
            draw_disassembly();
        }

        if (ImGui::CollapsingHeader("Memory", ImGuiTreeNodeFlags_DefaultOpen))
            memory_editor.DrawContents(memory.get(), memory->size());

        ImGui::End();
    }
};