#!/bin/bash

if [ "$CC" = "gcc" ]; then
	GCOV=gcov
	ARGS=""
else
	GCOV=llvm-cov
	ARGS=gcov
fi

/bin/bash <(curl -s https://codecov.io/bash) -x $GCOV -a $ARGS
