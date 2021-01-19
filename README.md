# Markov Huffman Coding

Welcome to a data compression project. This project applies Markov chains to Huffman coding,
creating an encoding system which takes better advantage of patterns.

### Table of Contents
- [Encoding Details](#encoding-details)
- [Existing work](#existing-work)
- [Performance](#performance)

## Encoding Details

Huffman coding is a data compression technique that uses variable-length codes to losslessly
compress data. By assigning short codewords to the most frequently used characters and longer
codewords to more uncommon characters, huffman coding saves space and creates an encoding tailored
to a use case. Even with a codec where characters have a uniform frequency (such as the digits of
pi), huffman coding may still save on space by providing a more compact encoding.

Here are a couple small examples with an alphabet `{a, b, c, d}` and a default 2-bits-per-character
fixed-length encoding.

```
source: aaaabbc
  7
 / \
a   3
   / \
  b   c
Fixed-length encoding: 00000000010110 (14 bits)
Huffman coding:        0000101011     (10 bits)
```

There are some situations where huffman coding may not be able do do anything, though. Another
example with the same alphabet and encoding:

```
source: abcdabcd
      8
  +---+---+
  4       4
 / \     / \
a   b   c   d
Fixed-length encoding: 0001101100011011 (16 bits)
Huffman coding:        0001101100011011 (16 bits)
```

Because every character in the alphabet is used with the same frequency, huffman coding is not able
to do anything here. The huffman coding and fixed-length encodings are identical.

But there's an obvious pattern here: `abcd` is just repeated twice. Every `a` is followed by `b`,
every `b` by `c`, and so on. Applying a markov model to our input codec will allow us to take
advantage of more patterns in the input data. This markov model can be helpful for better encoding
human language.

```
source: abcdabcd
prev: ' ' prev: a prev: b prev: c prev: d
     1        2       2       2       1
    /        /       /       /       /
   a        b       c       d       a
Fixed-length encoding: 0001101100011011 (16 bits)
Markov-Huffman coding: 00000000         (8 bits)
```

## Existing work

Unsurprisingly, this idea isn't a first. I've found a paper from 1985, [Markov-Huffman coding of LPC
parameters][1], and a stackoverflow post from 2018, [Huffman Coding for Markov Chain based on
conditional distribution][2], describing similar ideas. Both of these discuss taking advantage of
conditional probabilities with huffman coding in the context of engineering problems.

As far as I can tell, this project is the first to apply it to text compression.

## Performance

This project is a proof of concept. There is certainly room for performance improvement in the
implementation of this algorithm.

One big target for performance improvement is in the codeword decoder which could use a lookup
table. A simple 8-bit or 16-bit lookup table would be feasible, however, there could conceivably be
cases where there are rare codewords longer than 8 or 16 bits. Setting up a lookup table that can
work in all scenarios would require special handling for codewords exceeding the table key size or,
more elegantly, creating a multi-level lookup table.

[1]: https://ieeexplore.ieee.org/document/1164545
[2]: https://stackoverflow.com/questions/49955585/huffman-coding-for-markov-chain-based-on-conditional-distribution
