#pragma once
#include <memory>
#include <iostream>

class IRBuilder
{
public:
    std::string ir;
    int reg_cnt = 0;

    std::string NewReg()
    {
        return "%" + std::to_string(reg_cnt++);
    }

    void Emit(const std::string &s)
    {
        ir += s;
    }

    std::string GetIR()
    {
        return ir;
    }
};
// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual std::string GenerateIR(IRBuilder &builder) const = 0;
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

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return func_def->GenerateIR(builder);
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

    std::string GenerateIR(IRBuilder &builder) const override
    {
        builder.Emit("fun @" + ident + "(): ");
        func_type->GenerateIR(builder);
        builder.Emit(" ");
        block->GenerateIR(builder);
        return "";
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

    std::string GenerateIR(IRBuilder &builder) const override
    {

        if (func_type == "int")
        {
            builder.Emit("i32");
            return "";
        }
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

    std::string GenerateIR(IRBuilder &builder) const override
    {
        std::string ret = "";
        builder.Emit("{\n");
        builder.Emit("%entry:\n");
        stmt->GenerateIR(builder);
        builder.Emit("}\n");
        return ret;
    }
};

class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "StmtAST { ";
        exp->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        std::string ret = exp->GenerateIR(builder);
        builder.Emit("  ret " + ret + "\n");
        return ret;
    }
};
class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> unar_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        unar_exp->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return unar_exp->GenerateIR(builder);
    }
};

class UnaryExpAST : public BaseAST
{
public:
    std::string unaryOp;
    std::unique_ptr<BaseAST> exp_ast;

    void Dump() const override
    {
        std::cout << "UnaryExp { ";
        std::cout << unaryOp;
        exp_ast->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        std::string value = exp_ast->GenerateIR(builder);
        if (unaryOp == "+")
        {
            return value;
        }
        else if (unaryOp == "-")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = sub 0, " + value + "\n");
            return reg;
        }
        else if (unaryOp == "!")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = eq " + value + ", 0" + +"\n");
            return reg;
        }
        else
        {
            if (unaryOp != "")
                abort();
            return value;
        }
    }
};

class PrimaryExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp_ast;
    void Dump() const override
    {
        std::cout << "PrimaryExp { ";
        exp_ast->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIR(builder);
    }
};

class ParenExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp_ast;
    void Dump() const override
    {
        std::cout << "ParenExp { ";
        std::cout << " ( ";
        exp_ast->Dump();
        std::cout << " ) ";
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIR(builder);
    }
};

class NumberExpAST : public BaseAST
{
public:
    int number;
    void Dump() const override
    {
        std::cout << "NumberExp { ";
        std::cout << number;
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return std::to_string(number);
    }
};