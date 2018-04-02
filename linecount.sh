#!/bin/sh


find . -name "*.[c|h]" | xargs cat | grep -v ^$ | wc -l

