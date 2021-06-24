#!/usr/bin/env bash
if grub-file --is-x86-multiboot $1; then
  echo -e "\e[32mPASS: $1 \e[0mmultiboot confirmed"
  exit 0
else
  echo -e "\e[31mFAIL: $1 \e[0mthe file is not multiboot"
  exit 1
fi
