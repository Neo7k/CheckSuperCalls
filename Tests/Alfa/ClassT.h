#pragma once

template< class Type >
class T : public alpha::B
{
protected:

	void Func_HeaderDefined()
	{
		return 0;
	}
	
private:
	friend class alpha::beta::A;
	Type* t;
};

template< >
class T < bool > : public alpha::B
{
protected:

	int Func_HeaderDefined()
	{
		//return B::Func_HeaderDefined();
	}
	
private:
	friend class alpha::beta::A;
	Type* t;
};