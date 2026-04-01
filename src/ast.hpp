#pragma once
#include <memory>
#include <iostream>

class IRValue
{
private:
    bool is_const;
    int const_val;
    std::string name;

public:
    IRValue(bool is_const, int const_val, std::string name)
    {
        this->is_const = is_const;
        this->const_val = const_val;
        this->name = name;
    }
    std::string ToString()
    {
        if (is_const)
        {
            return std::to_string(const_val);
        }
        else
        {
            return name;
        }
    }
};

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
    virtual IRValue GenerateIRExpr(IRBuilder &) const { return IRValue(false, 0, ""); }
    virtual void GenerateIRStmt(IRBuilder &) const {}
};

class BaseExpAST : public BaseAST
{
public:
    virtual ~BaseExpAST() = default;
    virtual void Dump() const = 0;
    virtual IRValue GenerateIRExpr(IRBuilder &) const = 0;
};

class BaseStmtAST : public BaseAST
{
public:
    virtual ~BaseStmtAST() = default;
    virtual void Dump() const = 0;
    virtual void GenerateIRStmt(IRBuilder &) const = 0;
};

// CompUnit 是 BaseStmtAST
class CompUnitAST : public BaseStmtAST
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

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        func_def->GenerateIRStmt(builder);
    }
};

// FuncDef 也是 BaseStmtAST
class FuncDefAST : public BaseStmtAST
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

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        builder.Emit("fun @" + ident + "(): ");
        func_type->GenerateIRStmt(builder);
        builder.Emit(" ");
        block->GenerateIRStmt(builder);
    }
};

class FuncTypeAST : public BaseStmtAST
{
public:
    std::string func_type;
    void Dump() const override
    {
        std::cout << "FuncTypeAST { ";
        std::cout << func_type;
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {

        if (func_type == "int")
        {
            builder.Emit("i32");
        }
        else
            abort();
    }
};

class BlockAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    void Dump() const override
    {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        builder.Emit("{\n");
        builder.Emit("%entry:\n");
        stmt->GenerateIRStmt(builder);
        builder.Emit("}\n");
    }
};

class StmtAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "StmtAST { ";
        exp->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        std::string ret = exp->GenerateIRExpr(builder).ToString();
        builder.Emit("  ret " + ret + "\n");
    }
};
class ExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        lor_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return lor_exp->GenerateIRExpr(builder);
    }
};

class MulExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> mul_exp;
    std::string op;
    std::unique_ptr<BaseAST> unar_exp;

    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            mul_exp->Dump();
            std::cout << op << " ";
        }
        unar_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return unar_exp->GenerateIRExpr(builder);
        }
        else if (op == "*")
        {
            std::string mul_exp_value = mul_exp->GenerateIRExpr(builder).ToString();
            std::string unar_exp_value = unar_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = mul " + mul_exp_value + ", " + unar_exp_value + "\n");
            return {false, 0, reg};
        }
        else if (op == "/")
        {
            std::string mul_exp_value = mul_exp->GenerateIRExpr(builder).ToString();
            std::string unar_exp_value = unar_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = div " + mul_exp_value + ", " + unar_exp_value + "\n");
            return {false, 0, reg};
        }
        else if (op == "%")
        {
            std::string mul_exp_value = mul_exp->GenerateIRExpr(builder).ToString();
            std::string unar_exp_value = unar_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = mod " + mul_exp_value + ", " + unar_exp_value + "\n");
            return {false, 0, reg};
        }
        else
        {
            abort();
        }
    }
};

class AddExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> add_exp;
    std::string op;
    std::unique_ptr<BaseAST> mul_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            add_exp->Dump();
            std::cout << op << " ";
        }
        mul_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return mul_exp->GenerateIRExpr(builder);
        }
        else if (op == "+")
        {
            std::string mul_exp_value = add_exp->GenerateIRExpr(builder).ToString();
            std::string unar_exp_value = mul_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = add " + mul_exp_value + ", " + unar_exp_value + "\n");
            return {false, 0, reg};
        }
        else if (op == "-")
        {
            std::string mul_exp_value = add_exp->GenerateIRExpr(builder).ToString();
            std::string unar_exp_value = mul_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = sub " + mul_exp_value + ", " + unar_exp_value + "\n");
            return {false, 0, reg};
        }
        else
        {
            abort();
        }
    }
};

class RelExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> rel_exp;
    std::string op;
    std::unique_ptr<BaseAST> add_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            rel_exp->Dump();
            std::cout << op << " ";
        }
        add_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return add_exp->GenerateIRExpr(builder);
        }
        else if (op == "<")
        {
            std::string rel_exp_value = rel_exp->GenerateIRExpr(builder).ToString();
            std::string add_exp_value = add_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = lt " + rel_exp_value + ", " + add_exp_value + "\n");
            return {false, 0, reg};
        }
        else if (op == ">")
        {
            std::string rel_exp_value = rel_exp->GenerateIRExpr(builder).ToString();
            std::string add_exp_value = add_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = gt " + rel_exp_value + ", " + add_exp_value + "\n");
            return {false, 0, reg};
        }
        else if (op == "<=")
        {
            std::string rel_exp_value = rel_exp->GenerateIRExpr(builder).ToString();
            std::string add_exp_value = add_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = le " + rel_exp_value + ", " + add_exp_value + "\n");
            return {false, 0, reg};
        }
        else if (op == ">=")
        {
            std::string rel_exp_value = rel_exp->GenerateIRExpr(builder).ToString();
            std::string add_exp_value = add_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = ge " + rel_exp_value + ", " + add_exp_value + "\n");
            return {false, 0, reg};
        }
        else
        {
            abort();
        }
    }
};

class EqExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> eq_exp;
    std::string op;
    std::unique_ptr<BaseAST> rel_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            eq_exp->Dump();
            std::cout << op << " ";
        }
        rel_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return rel_exp->GenerateIRExpr(builder);
        }
        else if (op == "==")
        {
            std::string eq_exp_value = eq_exp->GenerateIRExpr(builder).ToString();
            std::string rel_exp_value = rel_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = eq " + eq_exp_value + ", " + rel_exp_value + "\n");
            return {false, 0, reg};
        }
        else if (op == "!=")
        {
            std::string eq_exp_value = eq_exp->GenerateIRExpr(builder).ToString();
            std::string rel_exp_value = rel_exp->GenerateIRExpr(builder).ToString();
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = ne " + eq_exp_value + ", " + rel_exp_value + "\n");
            return {false, 0, reg};
        }
        else
        {
            abort();
        }
    }
};

class LAndExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> land_exp;
    std::string op;
    std::unique_ptr<BaseAST> eq_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            land_exp->Dump();
            std::cout << "&& ";
        }
        eq_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return eq_exp->GenerateIRExpr(builder);
        }
        else if (op == "&&")
        {
            std::string land_exp_value = land_exp->GenerateIRExpr(builder).ToString();
            std::string eq_exp_value = eq_exp->GenerateIRExpr(builder).ToString();
            std::string reg1 = builder.NewReg();
            builder.Emit("  " + reg1 + " = ne " + land_exp_value + ", " + "0" + "\n");
            std::string reg2 = builder.NewReg();
            builder.Emit("  " + reg2 + " = ne " + eq_exp_value + ", " + "0" + "\n");
            std::string reg3 = builder.NewReg();
            builder.Emit("  " + reg3 + " = and " + reg1 + ", " + reg2 + "\n");
            return {false, 0, reg3};
        }
        else
        {
            abort();
        }
    }
};

class LOrExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;
    std::string op;
    std::unique_ptr<BaseAST> land_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            lor_exp->Dump();
            std::cout << "or ";
        }
        land_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return land_exp->GenerateIRExpr(builder);
        }
        else if (op == "||")
        {

            std::string lor_exp_value = lor_exp->GenerateIRExpr(builder).ToString();
            std::string land_exp_value = land_exp->GenerateIRExpr(builder).ToString();
            std::string reg1 = builder.NewReg();
            builder.Emit("  " + reg1 + " = ne " + lor_exp_value + ", " + "0" + "\n");
            std::string reg2 = builder.NewReg();
            builder.Emit("  " + reg2 + " = ne " + land_exp_value + ", " + "0" + "\n");
            std::string reg3 = builder.NewReg();
            builder.Emit("  " + reg3 + " = or " + reg1 + ", " + reg2 + "\n");
            return {false, 0, reg3};
        }
        else
        {
            abort();
        }
    }
};

class UnaryExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        IRValue ir_value = exp_ast->GenerateIRExpr(builder);
        std::string value = ir_value.ToString();
        if (unaryOp == "+")
        {
            return ir_value;
        }
        else if (unaryOp == "-")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = sub 0, " + value + "\n");
            return {false, 0, reg};
        }
        else if (unaryOp == "!")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = eq " + value + ", 0" + +"\n");
            return {false, 0, reg};
        }
        else
        {
            if (unaryOp != "")
                abort();
            return ir_value;
        }
    }
};

class PrimaryExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> exp_ast;
    void Dump() const override
    {
        std::cout << "PrimaryExp { ";
        exp_ast->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIRExpr(builder);
    }
};

class ParenExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIRExpr(builder);
    }
};

class NumberExpAST : public BaseExpAST
{
public:
    int number;
    void Dump() const override
    {
        std::cout << "NumberExp { ";
        std::cout << number;
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return {true, number, ""};
    }
};