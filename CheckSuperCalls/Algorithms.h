#pragma once
#include "Defs.h"

template <typename TContainer>
typename TContainer::const_iterator Find(const TContainer& container, const typename TContainer::value_type& value)
{
	return std::find(container.begin(), container.end(), value);
}

template <typename TContainer>
typename TContainer::iterator Find(TContainer& container, const typename TContainer::value_type& value)
{
	return std::find(container.begin(), container.end(), value);
}

template <typename TContainer, typename TDelegate>
typename TContainer::iterator FindIf(TContainer& container, const TDelegate& func)
{
	return std::find_if(container.begin(), container.end(), func);
}

template <typename TContainer, typename TDelegate>
typename TContainer::const_iterator FindIf(const TContainer& container, const TDelegate& func)
{
	return std::find_if(container.begin(), container.end(), func);
}

template <typename TContainer>
void Erase(TContainer& container, typename TContainer::value_type value)
{
	container.erase(std::remove(container.begin(), container.end(), value), container.end());
}