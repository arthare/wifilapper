#include "string.h"
#include <iostream>
#include "math.h"

using namespace std;

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) > (b) ? (b) : (a))

class BigNum
{
public:
	BigNum(const char* pszString)
	{
		cDigits = strlen(pszString);
		pDigits = new int[cDigits];

		for(int ixDigit = 0; ixDigit < cDigits; ixDigit++)
		{
			pDigits[ixDigit] = (pszString[ixDigit] - '0');
		}
	}
	BigNum(int cDigits)
	{
		this->cDigits = cDigits;
		pDigits = new int[cDigits];
		for(int ixDigit = 0; ixDigit < cDigits; ixDigit++)
		{
			pDigits[ixDigit] = 0;
		}
	}
	BigNum(const BigNum& other)
	{
		CopyFrom(other);
		
	}
	~BigNum()
	{
		if(pDigits)
		{
			delete pDigits;
			pDigits = 0;
			cDigits = 0;
		}
	}
	int GetDigitAt(int ixPower) const
	{
		if(ixPower <= cDigits - 1)
		{
			// ixPower is the power of "10" location that they want
			return pDigits[cDigits - ixPower - 1];
		}
		return 0;
	}
	void AddToDigit(int ixPower, int iValue)
	{
		int current = GetDigitAt(ixPower);
		int added = current + iValue;
		if(cDigits - ixPower - 1 < 0) __asm int 3;

		if(added >= 10)
		{
			AddToDigit(ixPower + 1,added / 10);
			added -= (10 * (added / 10));
			pDigits[cDigits - ixPower - 1] = added;
		}
		else
		{
			pDigits[cDigits - ixPower - 1] = added;
		}
	}
	const BigNum& operator =(const BigNum& other)
	{
		CopyFrom(other);
		return *this;
	}
	BigNum operator +(const BigNum& other) const
	{
		int maxPower = max(other.cDigits,cDigits);
		int minPower = min(other.cDigits,cDigits);
		BigNum result(maxPower+1);
		// go through ones, tens, hundreds, etc
		for(int ixPower = 0; ixPower < maxPower; ixPower++)
		{
			int iSum = other.GetDigitAt(ixPower) + GetDigitAt(ixPower);
			result.AddToDigit(ixPower,iSum);
		}

		WriteString();
		cout<<"plus"<<endl;
		other.WriteString();
		cout<<"equals"<<endl;
		result.WriteString();

		return result;
	}
	BigNum operator * (const int other) const
	{
		BigNum result(cDigits + (int)log((double)other) + 1);
		for(int ixPower = 0; ixPower < cDigits; ixPower++)
		{
			int iResult = GetDigitAt(ixPower) * other;
			result.AddToDigit(ixPower,iResult);
		}
		return result;
	}
	void WriteString() const
	{
		bool fWrittenFirstDigit = false;
		for(int x = 0; x < cDigits;x++)
		{
			if(pDigits[x] != 0 || fWrittenFirstDigit)
			{
				cout<<pDigits[x];
				fWrittenFirstDigit = true;
			}
		}
		cout<<endl;
	}
	int GetDigitSum() const
	{
		int sum = 0;
		for(int x = 0;x < cDigits; x++)
		{
			sum += pDigits[x];
		}
		return sum;
	}
private:
	void CopyFrom(const BigNum& other)
	{
		pDigits = new int[other.cDigits];
		cDigits = other.cDigits;
		for(int ix = 0; ix < cDigits;ix++)
		{
			pDigits[ix] = other.pDigits[ix];
		}
	}
private:
	int* pDigits;
	int cDigits;
};
