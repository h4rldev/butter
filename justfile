set shell := ["bash", "-c"]

## Metadata

lib_name := "libbutter"
test_name := "butter-test" # Using bread

## Directories

src := 'src'
include := 'include'
out := 'out'
lib := 'lib'
bin := 'bin'

## Type dirs

test_out := out + '/test'
butter_out := out + '/butter'
test_src := src + '/test'
butter_src := src + '/butter'

## General flags

include_flags := '-I' + include
link_flags := '-lhtils -lvulkan -Llib -ldl'

## Shared flags

shared_flags_debug := '-ggdb -g -Og -fsanitize=address,undefined,leak -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-common -DBUTTER_DEBUG'
shared_flags_release := '-O3 -std=gnu11'

## Link flags

release_wayland_link_flags := shared_flags_release + ' -lwayland-client -lxkbcommon -lbutter-wayland-release -lbread-wayland-release ' + link_flags
release_x11_link_flags := shared_flags_release + ' -lxkbcommon -lxkbcommon-x11 -lxcb -lxcb-icccm -lxcb-randr -lbutter-x11-release -lbread-x11-release ' + link_flags
debug_wayland_link_flags := shared_flags_debug + ' -lwayland-client -lxkbcommon -lbutter-wayland-debug -lbread-wayland-release ' + link_flags
debug_x11_link_flags := shared_flags_debug + ' -lxkbcommon -lxkbcommon-x11 -lxcb -lxcb-icccm -lxcb-randr -lbutter-x11-debug -lbread-x11-release ' + link_flags

## Static link flags

release_static_link_flags := shared_flags_release + ' -static -fPIC'
debug_static_link_flags := shared_flags_debug + ' -static -fPIC'

## Compile flags

release_compile_flags := shared_flags_release
debug_compile_flags := shared_flags_debug + ' -Wall -Wextra -Wpedantic -Wno-unused-parameter'
wayland_compile_flags := '-DBUTTER_WAYLAND'
x11_compile_flags := '-DBUTTER_X11'

## Colors

red := "\\x1b[31m"
green := "\\x1b[32m"
yellow := "\\x1b[33m"
reset := "\\x1b[0m"

default:
    just --list

## Utilities

clean:
    ! [[ -d {{ out }} ]] || rm -fr {{ out }}
    ! [[ -d {{ bin }} ]] || rm -fr {{ bin }}
    ! [[ -d {{ lib }} ]] || rm -fr {{ lib }}

bear:
    bear -- just compile-butter debug force

## Compile

compile-butter platform="wayland" target="debug" force="dont_force" threads=num_cpus():
    #!/usr/bin/env bash
    shopt -s globstar

    CURRENT_TARGET_COMPILE_FLAGS=""
    CURRENT_PLATFORM_COMPILE_FLAGS=""

    [[ -d {{ src }} ]] || exit 0


    if [[ {{ target }} == "release" ]]; then
      CURRENT_TARGET_COMPILE_FLAGS="{{ release_compile_flags }}"
    else
      CURRENT_TARGET_COMPILE_FLAGS="{{ debug_compile_flags }}"
    fi

    if [[ {{ platform }} == "wayland" ]]; then
        CURRENT_PLATFORM_COMPILE_FLAGS="{{ wayland_compile_flags }}"
    else
        CURRENT_PLATFORM_COMPILE_FLAGS="{{ x11_compile_flags }}"
    fi

    [[ -d {{ butter_out }} ]] || mkdir -p {{ butter_out }}

    WILL_COMPILE=false

    export WILL_COMPILE
    export CURRENT_PLATFORM_COMPILE_FLAGS
    export CURRENT_TARGET_COMPILE_FLAGS

    check_flags() {
      if [[ {{ force }} == "force" || {{ force }} == "true" ]]; then
        WILL_COMPILE=true; echo -e "Compile: Forcing"; return
      fi

      for file in {{ butter_src }}/**/*.c; do
        local current_out_file="$(basename "${file%.c}")-{{ platform }}-{{ target }}.o"


        if ! [[ -f {{ butter_out }}/${current_out_file} ]]; then
          WILL_COMPILE=true; return
        fi
        if [[ $file -nt {{ butter_out }}/${current_out_file} ]]; then
          WILL_COMPILE=true; return
        fi
      done
    }

    compile() {
      local file="$1"
      local current_out_file="$(basename "${file%.c}")-{{ platform }}-{{ target }}.o"

      if [[ "$file" == *"wayland"* && {{ platform }} != "wayland" ]]; then
        return
      fi

      if [[ "$file" == *"x11"* && {{ platform }} != "x11" ]]; then
        return
      fi

      echo -e "Compiling {{ green }}$file{{ reset }}..."
      gcc {{ include_flags }} ${CURRENT_TARGET_COMPILE_FLAGS} ${CURRENT_PLATFORM_COMPILE_FLAGS} -c "$file" -o "{{ butter_out }}/${current_out_file}"
    }
    export -f compile
    check_flags

    if [[ ${WILL_COMPILE} == false ]]; then
      echo -e "Compile (butter): Nothing to do"
      exit 0
    fi

    echo -e "Using {{ red }}{{ threads }}{{ reset }} threads"
    echo -e "Target: {{ green }}{{ target }}{{ reset }}"
    echo -e "Platform: {{ green }}{{ platform }}{{ reset }}"

    find {{ butter_src }} -name "*.c" -print0 | xargs -0 -P{{ threads }} -n1 bash -c 'compile "$0"'

    echo -e "Compile (butter): Compiling {{ green }}{{ target }}{{ reset }} complete"

assemble-butter platform="wayland" target="debug" static="dynamic" force="dont_force":
    #!/usr/bin/env bash
    shopt -s globstar

    [[ -d {{ butter_out }} ]] || just compile-butter {{ platform }} {{ target }} {{ force }}
    [[ -d {{ lib }} ]] || mkdir -p {{ lib }}

    ASSEMBLE_STATIC=false
    WILL_ASSEMBLE=false

    if [[ {{ target }} == "debug" ]]; then
      ASSEMBLE_STATIC=true
    elif [[ {{ target }} == "release" && {{ static }} == "static" || {{ static }} == "true" ]]; then
      ASSEMBLE_STATIC=true
    else
      ASSEMBLE_STATIC=false
    fi

    export ASSEMBLE_STATIC

    check_flags() {
      if [[ {{ force }} == "true" || {{ force }} == "force" ]]; then
        WILL_ASSEMBLE=true; echo -e "Assemble: Forcing"; return
      fi

      if ! [[ -f {{ lib }}/{{ lib_name }}-{{ platform }}-{{ target }}.a ]] && [[ ${ASSEMBLE_STATIC} == "true" ]]; then
        WILL_ASSEMBLE=true; return
      fi

      if ! [[ -f {{ lib }}/{{ lib_name }}-{{ platform }}-{{ target }}.so ]] && [[ ${ASSEMBLE_STATIC} == "false" ]]; then
        WILL_ASSEMBLE=true; return
      fi

      for file in {{ butter_out }}/*-{{ platform }}-{{ target }}.o; do
        if [[ "${ASSEMBLE_STATIC}" == "true" ]]; then
          if [[ $file -nt {{ lib }}/{{ lib_name }}-{{ platform }}-{{ target }}.a ]]; then
            WILL_ASSEMBLE=true; return
          fi
        else
          if [[ $file -nt {{ lib }}/{{ lib_name }}-{{ platform }}-{{ target }}.so ]]; then
            WILL_ASSEMBLE=true; return
          fi
        fi
      done
    }

    check_flags

    if [[ $WILL_ASSEMBLE == false ]]; then
      echo -e "Assemble (butter): Nothing to do"
      exit 0
    fi

    echo -e "Assemble (butter)"
    echo -e "Assembling {{ green }}{{ target }}{{ reset }}..."
    echo -e "Platform: {{ green }}{{ platform }}{{ reset }}"

    FILES=$(find {{ butter_out }} -name "*-{{ platform }}-{{ target }}.o")
    echo -e "Files to assemble: {{ green }}"
    printf "%s\n" "${FILES[@]}" | sed 's/^/    /'
    echo -e "{{ reset }}"

    if [[ "${ASSEMBLE_STATIC}" == "true" ]]; then
      ar rcs {{ lib }}/{{ lib_name }}-{{ platform }}-{{ target }}.a {{ butter_out }}/*-{{ platform }}-{{ target }}.o
      if [[ {{ target }} == "release" ]]; then
        strip --strip-debug {{ lib }}/{{ lib_name }}-{{ platform }}-{{ target }}.a
      fi
      ranlib {{ lib }}/{{ lib_name }}-{{ platform }}-{{ target }}.a
    else
      gcc -shared -o {{ lib }}/{{ lib_name }}-{{ platform }}-{{ target }}.so {{ butter_out }}/*-{{ platform }}-{{ target }}.o
      strip {{ lib }}/{{ lib_name }}-{{ platform }}-{{ target }}.so
    fi

    echo -e "Assemble (butter): Assembling {{ green }}{{ target }}{{ reset }} on {{ green }}{{ platform }}{{ reset }} complete"

compile-test platform="wayland" target="debug" force="dont_force" threads=num_cpus():
    #!/usr/bin/env bash
    shopt -s globstar

    [[ -d {{ test_out }} ]] || mkdir -p {{ test_out }}
    [[ -d {{ lib }} ]] || just debug

    WILL_COMPILE=false
    CURRENT_PLATFORM_COMPILE_FLAGS=""
    CURRENT_TARGET_COMPILE_FLAGS=""

    if [[ {{ platform }} == "wayland" ]]; then
      CURRENT_PLATFORM_COMPILE_FLAGS="{{ wayland_compile_flags }}"
    else
      CURRENT_PLATFORM_COMPILE_FLAGS="{{ x11_compile_flags }}"
    fi

    if [[ {{ target }} == "release" ]]; then
      CURRENT_TARGET_COMPILE_FLAGS="{{ release_compile_flags }}"
    else
      CURRENT_TARGET_COMPILE_FLAGS="{{ debug_compile_flags }}"
    fi

    export WILL_COMPILE
    export CURRENT_PLATFORM_COMPILE_FLAGS
    export CURRENT_TARGET_COMPILE_FLAGS

    check_flags() {
      if [[ {{ force }} == "true" || {{ force }} == "force" ]]; then
        WILL_COMPILE=true; echo -e "Compile: Forcing"; return
      fi

      if ! [[ -d {{ test_out }} ]]; then
        WILL_COMPILE=true; return
      fi

      for file in {{ test_src }}/*.c; do
        current_out_file="$(basename "${file%.c}").o"
        if ! [[ -f {{ test_out }}/${current_out_file} ]]; then
          WILL_COMPILE=true; return
        fi

        if [[ $file -nt {{ test_out }}/${current_out_file} ]]; then
          WILL_COMPILE=true; return
        fi
      done
    }

    check_flags

    if [[ ${WILL_COMPILE} == false ]]; then
      echo -e "Compile (test): Nothing to do"
      exit 0
    fi

    compile() {
      local file="$1"
      local current_out_file="$(basename "${file%.c}")-{{ platform }}.o"

      mkdir -p {{ test_out }}/$(basename "${file%.c}")

      echo -e "Compiling {{ green }}$file{{ reset }}..."
      gcc {{ include_flags }} ${CURRENT_TARGET_COMPILE_FLAGS} ${CURRENT_PLATFORM_COMPILE_FLAGS} -c "$file" -o "{{ test_out }}/$(basename ${file%.c})/${current_out_file}"
    }

    export -f compile

    echo -e "Compile (test):"
    echo -e "Platform: {{ green }}{{ platform }}{{ reset }}"
    echo -e "Using {{ red }}{{ threads }}{{ reset }} threads"

    find {{ src }}/test -name "*.c" -print0 | xargs -0 -P{{ threads }} -n1 bash -c 'compile "$0"'

    echo -e "Compile (test): Compiling {{ green }}{{ platform }}{{ reset }} complete"

link-test platform="wayland" target="debug" force="dont_force":
    #!/usr/bin/env bash
    shopt -s globstar
    set -x

    [[ -d {{ test_out }} ]] || just compile-test {{ platform }} {{ force }}
    [[ -d {{ bin }} ]] || mkdir -p {{ bin }}

    # Variable for checking if we should link.
    WILL_LINK=false
    LINK_STATIC=false

    CURRENT_PLATFORM_LINK_FLAGS=""

    if [[ {{ platform }} == "wayland" ]]; then
      if [[ {{ target }} == "release" ]]; then
        CURRENT_PLATFORM_LINK_FLAGS="{{ release_wayland_link_flags }}"
      else
        CURRENT_PLATFORM_LINK_FLAGS="{{ debug_wayland_link_flags }}"
      fi
    else
      if [[ {{ target }} == "release" ]]; then
        CURRENT_PLATFORM_LINK_FLAGS="{{ release_x11_link_flags }}"
      else
        CURRENT_PLATFORM_LINK_FLAGS="{{ debug_x11_link_flags }}"
      fi
    fi

    export CURRENT_PLATFORM_LINK_FLAGS
    export WILL_LINK

    check_flags() {
      if [[ {{ force }} == "true" || {{ force }} == "force" ]]; then
        WILL_LINK=true; echo -e "Link: Forcing"; return
      fi

      if ! [[ -f {{ bin }}/{{ test_name }}-{{ platform }} ]]; then
        WILL_LINK=true; return
      fi

      for file in {{ test_out }}/**/*-{{ platform }}.o; do
        if [[ $file -nt {{ bin }}/{{ test_name }}-{{ platform }} ]]; then
          WILL_LINK=true; return
        fi
      done
    }

    link() {
      echo -e "Link (test):"
      echo -e "Target: {{ green }}{{ target }}{{ reset }}"
      echo -e "Platform: {{ green }}{{ platform }}{{ reset }}"

      local file="$1"
      local out_sub=$(basename "${file%.o}")

      gcc $file ${CURRENT_PLATFORM_LINK_FLAGS} -o {{ bin }}/butter-test-$out_sub -fuse-ld=mold
    }

    check_flags

    if [[ ${WILL_LINK} == false ]]; then
      echo -e "Link: Nothing to do"
      exit 0
    fi
    for file in {{ test_out }}/**/*-{{ platform }}.o; do
      link $file
    done

## Aliases

build-butter platform="wayland" target="debug" force="false" static="dynamic" threads=num_cpus():
    just compile-butter {{ platform }} {{ target }} {{ force }} {{ threads }}
    just assemble-butter {{ platform }} {{ target }} {{ static }} {{ force }}

build-test platform="wayland" target="debug" force="false" threads=num_cpus():
    just {{ target }} {{ platform }} {{ force }} static {{ threads }}

    just compile-test {{ platform }} {{ target }} {{ force }} {{ threads }}
    just link-test {{ platform }} {{ target }} {{ force }}

release platform="wayland" force="false" static="dynamic" threads=num_cpus():
    just build-butter {{ platform }} release {{ force }} {{ static }} {{ threads }}

debug platform="wayland" force="false" static="dynamic" threads=num_cpus():
    just build-butter {{ platform }} debug {{ force }} {{ static }} {{ threads }}
