#! /bin/bash
echo "Building test..."
qmake -makefile -o test/Makefile src/wserial.pro -config test DEFINES+="LOGGING_MODE=QDEBUG"
cd test
make -j4
if [[ "$?" != "0" ]] ; then
    echo "Build failed!"
    exit 1
fi
lcov -d . -z
echo "Running test..."
./test
if [[ "$?" != "0" ]] ; then
    echo "One or more tests failed!"
    exit 1
fi
lcov -d . -c -o lcov.info
lcov -r lcov.info '/usr/*' '*/moc_*' '*/ui_*' '*/qrc_*' '*.h' '*tests/*' -o lcov.info
lcov -l lcov.info
genhtml -o ../coverage -t "Test Coverage" --num-spaces 4 lcov.info
