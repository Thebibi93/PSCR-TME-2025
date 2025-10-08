#include "String.h"
#include "strutil.h"
#include <cstring>

namespace pr {

// TODO: Implement constructor e.g. using initialization list
String::String(const char *s) {
  std::cout << "String constructor called for: " << s << std::endl;
  // Allocate and copy string data
  data = newcopy(s ? s : "");
}

String::~String() {
  std::cout << "String destructor called for: " << (data ? data : "(null)")
            << std::endl;

  // Free allocated string data
  delete[] data;
  data = nullptr; // Avoid accessing freed memory
}

// TODO : add other operators and functions

String::String(const String &other) {
  std::cout << "String copy constructor called for: "
            << (data ? data : "(null)") << std::endl;
  data = newcopy(other.data);
}

String &String::operator=(const String &other) {
  std::cout << "String copy assignment operator called for: "
            << (data ? data : "(null)") << std::endl;
  if (this != &other) {         // Self-assignment check
    delete[] data;              // Free existing resource
    data = newcopy(other.data); // Allocate and copy new resource
  }
  return *this;
}

// Move constructor
String::String(String &&other) noexcept {
  std::cout << "String move constructor called for: "
            << (other.data ? other.data : "(null)") << std::endl;
  data = other.data;
  other.data = nullptr;
}

// Move assign
String &String::operator=(String &&other) noexcept {
  std::cout << "String move assign called for: "
            << (other.data ? other.data : "(null)") << std::endl;
  if (this != &other) {
    delete[] data;
    data = other.data;
    other.data = nullptr;
  }
  return *this;
}

// Member for ordering
bool String::operator<(const String& other) const{
  return compare(this->data, other.data) < 0;
}

std::ostream& operator<<(std::ostream& os, const String& str) {
  os << (str.data ? str.data : "(null)");
  return os;
}

bool operator==(const String& a, const String& b){
  return compare(a.data, b.data) == 0;
}

String operator+(const String& a, const String& b){
  size_t len_a = length(a.data);
  size_t len_b = length(b.data);
  char *data = new char[len_a + len_b + 1];
  std::memcpy(data, a.data, len_a);
  std::memcpy(data + len_a, b.data, len_b);
  data[len_a + len_b] = '\0';
  String result(data);
  delete[] data;
  return result;
}

} // namespace pr