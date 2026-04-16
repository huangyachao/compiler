#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "koopa.h"

class Vistor
{
private:
    // enum class Type
    // {
    //     Int,
    //     Register,
    //     Variable,
    //     Unknown,
    // };

    // class Value
    // {
    // public:
    //     Type type;
    //     std::string data;

    //     Value()
    //     {
    //         type = Type::Unknown;
    //         data = "";
    //     }

    //     Value(Type type, std::string data)
    //     {
    //         this->type = type;
    //         this->data = data;
    //     }
    // };
    koopa_raw_program_builder_t builder;
    koopa_raw_program_t raw_program;
    std::vector<std::string> global_symbols;
    std::vector<std::string> registers = {
        "t0",
        "t1",
        "t2",
        "t3",
        "t4",
        "t5",
        "t6",
        "a0",
        "a1",
        "a2",
        "a3",
        "a4",
        "a5",
        "a6",
        "a7",
    };
    int register_count = 0;
    int stack_offset = 0;
    std::string riscv_code = "";
    std::unordered_map<koopa_raw_value_t, std::string> raw_value_map;
    std::string NewRegister()
    {
        return registers[register_count++];
    }

    void ResetRegister()
    {
        register_count = 0;
    }

    std::string AllocStack()
    {
        stack_offset += 4;
        return std::to_string(stack_offset - 4) + "(sp)";
    }

    int GetStackSize()
    {
        return stack_offset;
    }

    void ResetStack()
    {
        stack_offset = 0;
    }

public:
    Vistor(std::string ir)
    {

        // 解析字符串 ir, 得到 Koopa IR 程序
        koopa_program_t program;
        koopa_error_code_t error_code = koopa_parse_from_string(ir.c_str(), &program);
        assert(error_code == KOOPA_EC_SUCCESS); // 确保解析时没有出错
        // 创建一个 raw program builder, 用来构建 raw program
        builder = koopa_new_raw_program_builder();
        // 将 Koopa IR 程序转换为 raw program
        raw_program = koopa_build_raw_program(builder, program);
        // 释放 Koopa IR 程序占用的内存
        koopa_delete_program(program);
    }

    // 访问 raw program
    void Visit()
    {
        // 访问所有全局变量
        Visit(raw_program.values);
        // 访问所有函数
        Visit(raw_program.funcs);

        std::string new_riscv_code = "  .text\n";
        for (std::string symbol : global_symbols)
        {
            new_riscv_code += "  .globl " + symbol + "\n";
        }
        riscv_code = new_riscv_code + riscv_code;
        return;
    }

    // 访问 raw slice
    void Visit(const koopa_raw_slice_t &slice)
    {
        for (size_t i = 0; i < slice.len; ++i)
        {
            auto ptr = slice.buffer[i];
            // 根据 slice 的 kind 决定将 ptr 视作何种元素
            switch (slice.kind)
            {
            case KOOPA_RSIK_FUNCTION:
                // 访问函数
                Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 访问基本块
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
            {
                // 访问指令
                koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
                std::string ret = Visit(value);
                if (raw_value_map.count(value) == 0 && ret != "")
                {
                    raw_value_map[value] = ret;
                }
                ResetRegister();
                break;
            }
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
            }
        }
        return;
    }

    // 访问函数
    void Visit(const koopa_raw_function_t &func)
    {
        std::string symbol = std::string(func->name).substr(1);
        global_symbols.push_back(symbol);
        riscv_code += symbol + ":\n";
        std::string new_risc_code = riscv_code;
        riscv_code = "";
        // 访问所有基本块
        ResetStack();
        Visit(func->bbs);
        int offset = GetStackSize();
        new_risc_code += "  addi  sp, sp, -" + std::to_string(offset) + "\n";
        new_risc_code += riscv_code;
        new_risc_code += "  addi  sp, sp, " + std::to_string(offset) + "\n";
        new_risc_code += "  ret\n";
        riscv_code = new_risc_code;
        return;
    }

    // 访问基本块
    void Visit(const koopa_raw_basic_block_t &bb)
    {
        // 访问所有指令
        Visit(bb->insts);
    }

    // 访问指令
    std::string Visit(const koopa_raw_value_t &value)
    {

        std::string ret = "";
        // 根据指令类型判断后续需要如何访问
        const auto &kind = value->kind;
        switch (kind.tag)
        {
        case KOOPA_RVT_RETURN:
            // 访问 return 指令
            Visit(kind.data.ret);
            break;
        case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            ret = Visit(kind.data.integer);
            break;
        case KOOPA_RVT_BINARY:
            // 访问 binary 指令
            if (raw_value_map.count(value) > 0)
            {
                return raw_value_map[value];
            }
            ret = Visit(kind.data.binary);
            break;
        case KOOPA_RVT_ALLOC:
            // 访问 global_alloc 指令
            if (raw_value_map.count(value) > 0)
            {
                return raw_value_map[value];
            }
            ret = AllocStack();
            break;
        case KOOPA_RVT_STORE:
        {
            // 访问 store 指令
            std::string value = Visit(kind.data.store.value);
            if (value.length() > 4 && value.substr(value.length() - 4) == "(sp)")
            {
                std::string reg = NewRegister();
                riscv_code += "  lw    " + reg + ", " + value + "\n";
                value = reg;
            }
            std::string dest = Visit(kind.data.store.dest);
            riscv_code += "  sw    " + value + ", " + dest + "\n";
            break;
        }

        case KOOPA_RVT_LOAD:
        {
            // 访问 load 指令
            if (raw_value_map.count(value) > 0)
            {
                std::string reg = NewRegister();
                riscv_code += "  lw    " + reg + ", " + raw_value_map[value] + "\n";
                return reg;
            }
            std::string src = Visit(kind.data.load.src);
            std::string reg = NewRegister();
            riscv_code += "  lw    " + reg + ", " + src + "\n";
            ret = AllocStack();
            riscv_code += "  sw    " + reg + ", " + ret + "\n";
            break;
        }

        default:
            // 其他类型暂时遇不到
            assert(false);
        }

        return ret;
    }

    std::string Visit(const koopa_raw_integer_t &value)
    {
        std::string s_val = std::to_string(value.value);
        if (s_val == "0")
        {
            // return Value(Type::Register, "x0");
            return "x0";
        }
        std::string new_reg = NewRegister();
        riscv_code += "  li    " + new_reg + ", " + s_val + "\n";
        return new_reg;
        // return Value(Type::Register, new_reg);
    }

    void Visit(const koopa_raw_return_t &value)
    {
        koopa_raw_value_t raw_value = value.value;
        if (raw_value == nullptr)
            return;
        std::string return_value = Visit(raw_value);
        if (return_value.length() > 4 && return_value.substr(return_value.length() - 4) == "(sp)")
        {
            riscv_code += "  lw    a0, " + return_value + "\n";
        }
        else if (return_value != "")
        {
            riscv_code += "  mv    a0, " + return_value + "\n";
        }
        return;
    }

    std::string Visit(const koopa_raw_binary_t &binary)
    {
        std::string first_value = Visit(binary.lhs);
        std::string second_value = Visit(binary.rhs);

        if (first_value.length() > 4 && first_value.substr(first_value.length() - 4) == "(sp)")
        {
            std::string reg = NewRegister();
            riscv_code += "  lw    " + reg + ", " + first_value + "\n";
            first_value = reg;
        }

        if (second_value.length() > 4 && second_value.substr(second_value.length() - 4) == "(sp)")
        {
            std::string reg = NewRegister();
            riscv_code += "  lw    " + reg + ", " + second_value + "\n";
            second_value = reg;
        }

        std::string ret = "";
        if (binary.op == KOOPA_RBO_NOT_EQ)
        {
            ret = first_value;
            if (first_value == "x0")
            {
                ret = NewRegister();
            }
            riscv_code += "  xor   " + ret + ", " + first_value + ", " + second_value + "\n";
            riscv_code += "  snez  " + ret + ", " + ret + "\n";
        }
        else if (binary.op == KOOPA_RBO_EQ)
        {
            ret = first_value;
            if (first_value == "x0")
            {
                ret = NewRegister();
            }
            riscv_code += "  xor   " + ret + ", " + first_value + ", " + second_value + "\n";
            riscv_code += "  seqz  " + ret + ", " + ret + "\n";
        }
        else if (binary.op == KOOPA_RBO_GT)
        {
            ret = first_value;
            if (first_value == "x0")
            {
                ret = NewRegister();
            }
            riscv_code += "  sgt   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_LT)
        {
            ret = first_value;
            if (first_value == "x0")
            {
                ret = NewRegister();
            }
            riscv_code += "  slt   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_GE)
        {
            ret = first_value;
            if (first_value == "x0")
            {
                ret = NewRegister();
            }
            riscv_code += "  slt   " + ret + ", " + first_value + ", " + second_value + "\n";
            riscv_code += "  seqz  " + ret + ", " + ret + "\n";
        }
        else if (binary.op == KOOPA_RBO_LE)
        {
            ret = first_value;
            if (first_value == "x0")
            {
                ret = NewRegister();
            }
            riscv_code += "  sgt   " + ret + ", " + first_value + ", " + second_value + "\n";
            riscv_code += "  seqz  " + ret + ", " + ret + "\n";
        }
        else if (binary.op == KOOPA_RBO_ADD)
        {
            ret = NewRegister();
            riscv_code += "  add   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_SUB)
        {
            ret = NewRegister();
            riscv_code += "  sub   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_MUL)
        {
            ret = NewRegister();
            riscv_code += "  mul   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_DIV)
        {
            ret = NewRegister();
            riscv_code += "  div   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_MOD)
        {
            ret = NewRegister();
            riscv_code += "  rem   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_AND)
        {
            ret = NewRegister();
            riscv_code += "  and   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_OR)
        {
            ret = NewRegister();
            riscv_code += "  or    " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_XOR)
        {
            ret = NewRegister();
            riscv_code += "  xor   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_SHL)
        {
            ret = NewRegister();
            riscv_code += "  shl   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_SHL)
        {
            ret = NewRegister();
            riscv_code += "  shr   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        else if (binary.op == KOOPA_RBO_SAR)
        {
            ret = NewRegister();
            riscv_code += "  sar   " + ret + ", " + first_value + ", " + second_value + "\n";
        }
        std::string offset = AllocStack();
        riscv_code += "  sw    " + ret + ", " + offset + "\n";

        return offset;
    }

    void Visit(const koopa_raw_global_alloc_t &value)
    {
        koopa_raw_value_t raw_value = value.init;
        std::string return_value = Visit(raw_value);
        return;
    }

    std::string GetRiscvCode()
    {
        return riscv_code;
    }

    ~Vistor()
    {
        // 处理完成, 释放 raw program builder 占用的内存
        // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
        // 所以不要在 raw program 处理完毕之前释放 builder
        koopa_delete_raw_program_builder(builder);
    }
};