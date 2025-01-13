#include "libqbe.h"

int main() {
  lq_init(LQ_TARGET_DEFAULT, stdout, "");
  lq_shutdown();
}
