#include <iostream>

int main() {
  try {
    cout << "Hello World" << endl;
    exit(EXIT_SUCCESS);
  } catch (const exception &ex) {
    cerr << ex.what();
    exit(EXIT_FAILURE);
  }
}
