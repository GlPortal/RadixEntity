#ifndef RADIXENTITIES_ENTITY_HPP
#define RADIXENTITIES_ENTITY_HPP

#include <unordered_map>

#include "Method.hpp"
#include "Property.hpp"
#include "Signal.hpp"

namespace RadixEntities {

class Entity {
protected:
  friend class MethodBase;
  friend class PropertyBase;
  friend class SignalBase;

  std::unordered_map<std::string, MethodBase*> m_methods;
  std::unordered_map<std::string, PropertyBase*> m_properties;
  std::unordered_map<std::string, SignalBase*> m_signals;

public:
  Entity() {}
  virtual ~Entity();

  Entity(Entity&&) = delete;
  Entity& operator=(Entity&&) = delete;
};

} /* namespace RadixEntities */

#endif /* RADIXENTITIES_ENTITY_HPP */