#ifndef PACSTDUTIL_H
#define PACSTDUTIL_H 

#include "pacConsolePreRequisite.h"
namespace pac
{

class StdUtil
{
private:
	StdUtil(){}
public:

	/**
	 * Get kyes of a map
	 * @param t : map 
	 * @return : vector of map keys 
	 */
	template<class T>
	static std::vector<typename T::key_type> keys(const T& t)
	{
		std::vector<typename T::key_type> v;
		std::for_each(t.begin(), t.end(), [&](typename T::value_type val)->void
		{
			v.push_back(val.first);
		});

		return v;
	}

	template<class T>
	static bool exist(const T& t, const typename T::value_type& v)
	{
		return std::find(t.begin(), t.end(), v) != t.end();
	}
	
	/**
	 * Get identical string from string vector .
	 */
	static std::string getIdenticalString(StringVector::iterator beg,
			StringVector::iterator end);
	

};


}


#endif /* PACSTDUTIL_H */
