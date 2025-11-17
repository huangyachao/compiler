#pragma once
#include <memory>
#include <iostream>

// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual std::string GenerateIR() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override
    {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }

    std::string GenerateIR() const override
    {
        return func_def->GenerateIR();
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override
    {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", ";
        std::cout << ident << ", ";
        block->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR() const override
    {
        std::string ret = "fun @" + ident + "(): ";
        ret += func_type->GenerateIR();
        ret += " ";
        ret += block->GenerateIR();
        return ret;
    }
};

class FuncTypeAST : public BaseAST
{
public:
    std::string func_type;
    void Dump() const override
    {
        std::cout << "FuncTypeAST { ";
        std::cout << func_type;
        std::cout << " } ";
    }

    std::string GenerateIR() const override
    {

        if (func_type == "int")
            return "i32";
        else
            abort();
    }
};

class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    void Dump() const override
    {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR() const override
    {
        std::string ret = "{\n";
        ret += "%entry:\n";
        ret += stmt->GenerateIR();
        ret += "}\n";
        return ret;
    }
};

class StmtAST : public BaseAST
{
public:
    int number;
    void Dump() const override
    {
        std::cout << "StmtAST { ";
        std::cout << number;
        std::cout << " } ";
    }

    std::string GenerateIR() const override
    {
        std::string ret = "  ";
        ret += "ret ";
        ret += std::to_string(number) + "\n";
        return ret;
    }
};
