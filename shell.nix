{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  buildInputs = [
    pkgs.gcc
    pkgs.gdb
    pkgs.clang
  ];

  propagatedBuildInputs = [
  ];

  shellHook = ''
    export LIBRARY_PATH=$LIBRARY_PATH
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH
    export PATH=$PATH
    export C_INCLUDE_PATH=$C_INCLUDE_PATH
    export TMPDIR=$XDG_RUNIME_DIR
    unset WAYLAND_DISPLAY 
  '';
}
