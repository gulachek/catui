#include <catui.h>

int main() {
  catui_connect("com.example.test", "1.2.3", stderr);
  // don't actually care if above succeeds. Compiles/links/runs
  return 0;
}
