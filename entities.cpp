#include <cstdint>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

struct EntityIoValueType {
  uint32_t namespc;
  uint32_t type;
};


class PropertyBase {
  std::string name;
  EntityIoValueType valueType;
};

class MethodBase {
};


class Entity {
//protected:
public:
  std::unordered_map<std::string, PropertyBase*> m_properties;
};


enum PropertyAccess {
  R = 1,
  W = 2,
  RW = 3,
};

template<typename T, PropertyAccess A>
class Property : public PropertyBase {
public:
  // TODO: type-erased callbacks

  using ReadRawFuncPtr = std::remove_reference_t<T>&&(*)(Entity&);
  using ReadStdFuncPtr = std::function<std::remove_reference_t<T>&&(Entity&)>;

  // Could be changed to a T&& new value param
  using WriteRawFuncPtr = void(*)(Entity&, const T &newval);
  using WriteStdFuncPtr = std::function<void(Entity&, const T &newval)>;

  using OnChangeRawFuncPtr = void(*)(Entity&, const T &newval);
  using OnChangeStdFuncPtr = std::function<void(Entity&, const T &newval)>;

protected:
  const char *const m_name;
  Entity *m_container;
  T *m_valuePtr;
  OnChangeStdFuncPtr m_onChange;

  Property(const char *name, Entity *container) :
    m_name(name),
    m_container(container) {
    container->m_properties.emplace(std::piecewise_construct,
        std::forward_as_tuple(name),
        std::forward_as_tuple(this));
  }

public:
  Property(const char *name, Entity *container, T *valuePtr, OnChangeStdFuncPtr onChange = {}) :
    Property(name, container) {
    m_valuePtr = valuePtr;
    m_onChange = onChange;
  }

  operator const T&() const {
    static_assert(A | PropertyAccess::R, "Property has no read access");
    return *m_valuePtr;
  }

  template<typename C, typename = typename std::enable_if_t<std::is_convertible<C, T>::value>>
  const T& operator=(const C &v) {
    static_assert(A | PropertyAccess::W, "Property has no write access");
    //if (m_onChange) {
    //  m_onChange(
    //}
    *m_valuePtr = v;
    return *m_valuePtr;
  }
  template<typename C, typename = typename std::enable_if_t<std::is_convertible<C, T>::value>>
  const T& operator=(T &&v) {
    static_assert(A | PropertyAccess::W, "Property has no write access");
    *m_valuePtr = std::move(v);
    return *m_valuePtr;
  }
};

// TODO: synthetic properties (user get/set method)

template<class R, typename... Args>
class Method {};

template<class R, typename... Args>
class Method<R(Args...)> {
protected:
  std::function<R(Entity&, Args...)> m_func;
  const char *const m_name;
  Entity *m_container;

public:
  template<typename... CallArgs>
  Method(const char *name, Entity *container, CallArgs&&... ca) :
    m_func(std::forward<CallArgs>(ca)...),
    m_name(name),
    m_container(container) {
  }

  template<typename... CallArgs>
  R operator()(CallArgs&&... ca) const {
    return m_func(*m_container, std::forward<CallArgs>(ca)...);
  }
};

template<typename... Args>
class Signal {
public:
  using Func = std::function<void(Entity&, Args...)>;

protected:
  std::vector<Func> m_listeners;
  const char *const m_name;
  Entity *m_container;

public:
  Signal(const char *name, Entity *container) :
    m_name(name),
    m_container(container) {
  }

  // TODO make holder object to functions are automatically removed upon object death
  void addListener(const Func &func) {
    m_listeners.emplace_back(func);
  }

  template<typename... CallArgs>
  void operator()(CallArgs&&... ca) const {
    for (const Func &fn : m_listeners) {
      fn(*m_container, std::forward<CallArgs>(ca)...);
    }
  }
};

// EXAMPLE

struct Vector4 {
  double x, y, z, w;

  Vector4(double x, double y, double z, double w) :
    x(x),
    y(y),
    z(z),
    w(w) {
  }
};

class MyEntity : public /*virtual*/ Entity {
protected:
  int m_health;
  Vector4 m_remainingInk = Vector4(1, 2, 3, 4);

public:
  Property<int, PropertyAccess::RW> health;
  Property<Vector4, PropertyAccess::R> remainingInk;
  Method<int(int, int)> myMethod;
  Signal<int, int> signalr;

  MyEntity() :
    m_health(1337),
    health("health", this, &m_health),
    remainingInk("remainingInk", this, &m_remainingInk),
    myMethod("myMethod", this, &myMethodImpl),
    signalr("signalr", this) {
  }

  static int myMethodImpl(Entity &ent, int a, int b) {
    MyEntity &self = static_cast<MyEntity&>(ent);
    self.signalr(a + 2, b - 4);
    return 42;
  }
};

#include <assert.h>
#include <iostream>

class CustomThingy {
public:
  int val;

  CustomThingy(int v) :
    val(v) {
  }

  operator int() const {
    return val;
  }
};

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;
  MyEntity mayo;

  // Initial value
  assert(mayo.health == 1337);

  // Primitive same-type operator=
  mayo.health = 12;
  assert(mayo.health == 12);

  // Implicit cast other-type lvalue operator=
  CustomThingy ct(-237);
  mayo.health = ct;
  assert(mayo.health == -237);

  // Implicit cast other-type rvalue operator=
  mayo.health = CustomThingy(8);
  assert(mayo.health == 8);

  // Implicit cast other-type std::move'd operator=
  mayo.health = std::move(CustomThingy(33));
  assert(mayo.health == 33);


  mayo.signalr.addListener([](Entity&, int a, int b) {
    std::cout << a << ' ' << b << std::endl;
  });
  mayo.signalr.addListener([](Entity&, int a, int b) {
    std::cout << b << ' ' << a << std::endl;
  });
  mayo.signalr(12, 34);


  mayo.myMethod(0, 0);

  return 0;
}
