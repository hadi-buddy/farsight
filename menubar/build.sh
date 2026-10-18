#!/bin/zsh
# Builds the Farsight menu-bar language switcher.
cd ${0:A:h} && swiftc -O -o farsight-menubar main.swift
