## Summary

Polyglot is an open-source chess opening book format introduced by Fabien Letouzey. This project dervies from his original `PolyGlot 1.4` source code, and removes everything that is irrelevant to the book. You should use this project to generate Polyglot book from PGN file in command line.

## Compile

`gcc *.c -opolyglot`

## Usage

Download a sample PGN file:

`wget http://smallchess.com/Games/Magnus%20Carlsen.pgn`

Create a Polyglot book with the PGN file:

`polyglot MakeBook -pgn Magnus\ Carlsen.pgn -bin Carlsen.bin`
