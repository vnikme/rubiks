#!/usr/bin/python
# coding: utf-8

"""
       4  5
       7  6

20 21  0  1  16 17
23 22  3  2  19 18

      12 13
      15 14

       8  9
      11 10
"""

R = { 1: 5, 5: 9, 9: 13, 13: 1, 2: 6, 6: 10, 10: 14, 14: 2, 16: 17, 17: 18, 18: 19, 19: 16 }
L = { 0: 12, 12: 8, 8: 4, 4: 0, 3: 15, 15: 11, 11: 7, 7: 3, 21: 22, 22: 23, 23: 20, 20: 21 }
U = { 4: 5, 5: 6, 6: 7, 7: 4, 21: 11, 11: 17, 17: 1, 1: 21, 20: 10, 10: 16, 16: 0, 0: 20 }
D = { 12: 13, 13: 14, 14: 15, 15: 12, 3: 19, 19: 9, 9: 23, 23: 3, 2: 18, 18: 8, 8: 22, 22: 2 }
F = { 0: 1, 1: 2, 2: 3, 3: 0, 7: 16, 16: 13, 13: 22, 22: 7, 6: 19, 19: 12, 12: 21, 21: 6 }
B = { 8: 9, 9: 10, 10: 11, 11: 8, 4: 23, 23: 14, 14: 17, 17: 4, 5: 20, 20: 15, 15: 18, 18: 5 }


def compose(a, b):
    c = {}
    for i in xrange(24):
        t = a.get(i, i)
        t = b.get(t, t)
        if i != t:
            c[i] = t
    return c


def apply(cube, move):
    result = [' ' for i in xrange(24)]
    for i in xrange(24):
        to = move.get(i, i)
        result[to] = cube[i]
    return "".join(result)


def is_solved(cube):
    for i in xrange(6):
        for j in xrange(3):
            if cube[i * 4 + j] != cube[i * 4 + j + 1]:
                return False
    return True


moves = {
    "R": R, "L": L, "U": U, "D": D, "F": F, "B": B,
    "R2": compose(R, R), "L2": compose(L, L),
    "U2": compose(U, U), "D2": compose(D, D),
    "F2": compose(F, F), "B2": compose(B, B),
    "R'": compose(R, compose(R, R)), "L'": compose(L, compose(L, L)),
    "U'": compose(U, compose(U, U)), "D'": compose(D, compose(D, D)),
    "F'": compose(F, compose(F, F)), "B'": compose(B, compose(B, B))
}


def solve(cube):
    ans = {cube: []}
    q = [cube]
    i = 0
    while i < len(q):
        current = q[i]
        for m in moves:
            c = apply(current, moves[m])
            if c not in ans:
                q.append(c)
                ans[c] = ans[current] + [m]
                if len(ans) % 1000000 == 0:
                    print(len(ans), len(ans[c]), c)
                if is_solved(c):
                    return ans[c]
        i += 1
    return None


def main():
    cube = "brygoobwrygwoogwyyrbbrwg"
    #cube = "wwwwrrrryyyyooooggggbbbb"
    print(solve(cube))


if __name__ == "__main__":
    main()

