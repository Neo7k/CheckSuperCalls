#pragma once

template< class T >
class ClassT : public alpha::B
{
protected:

    void Func_HeaderDefined()
    {
        return 0;
    }
    
private:
    friend class alpha::beta::A;
    T* t;
};

template< >
class ClassT < bool > : public alpha::B
{
protected:

    void Func_HeaderDefined()
    {
        return B::Func_HeaderDefined();
    }
    
private:
    friend class alpha::beta::A;
    T* t;
};