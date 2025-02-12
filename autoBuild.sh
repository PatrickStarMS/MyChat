set -x

# rm -rf 'pwd'/build/*
# cd 'pwd' build &&
#     cmake .. &&
#     make
cmake -B build
cmake --build build