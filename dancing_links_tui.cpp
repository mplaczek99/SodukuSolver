#include <bits/stdc++.h>
#include <ncurses.h>

using namespace std;

// ------------------------------------------------------------------
//  Dancing Links (DLX) solver for Sudoku
// ------------------------------------------------------------------

// We'll rename these to avoid conflict with ncurses' COLS variable.
static const int N = 9;                    // 9x9 Sudoku
static constexpr int DLX_COLS = 4 * N * N; // 324 columns for constraints

struct DLXNode {
    DLXNode *L, *R, *U, *D;
    int colIndex;
    int rowIndex;
};

struct Column {
    DLXNode head;
    int size;
};

static const int MAX_NODES = 9 * 9 * 9 * 4 + 5000;
static DLXNode nodes[MAX_NODES];
static int nodeCount;
static Column dlxCols[DLX_COLS]; // renamed
static DLXNode root;
static vector<int> solutionRows;

// Forward declarations
static bool searchDLX(int depth);

inline int boxIndex(int r, int c) {
    return (r / 3) * 3 + (c / 3);
}

inline int encodeRowIndex(int r, int c, int d) {
    return r * 81 + c * 9 + d;
}

inline void candidateToCols(int r, int c, int d, int outCols[4]) {
    outCols[0] = r * 9 + c;                      // cell
    outCols[1] = 81 + (r * 9 + d);               // row-digit
    outCols[2] = 162 + (c * 9 + d);              // col-digit
    outCols[3] = 243 + (boxIndex(r, c) * 9 + d); // box-digit
}

// Link horizontally
inline void linkLR(DLXNode *left, DLXNode *right) {
    left->R = right;
    right->L = left;
}

// Link vertically
inline void linkUD(DLXNode *up, DLXNode *down) {
    up->D = down;
    down->U = up;
}

// Cover a column
static void cover(Column &c) {
    // Remove column header from root's LR list
    c.head.R->L = c.head.L;
    c.head.L->R = c.head.R;
    // Remove rows
    for (DLXNode *row = c.head.D; row != &c.head; row = row->D) {
        for (DLXNode *node = row->R; node != row; node = node->R) {
            node->U->D = node->D;
            node->D->U = node->U;
            dlxCols[node->colIndex].size--;
        }
    }
}

// Uncover a column
static void uncover(Column &c) {
    // Reinsert rows from bottom to top
    for (DLXNode *row = c.head.U; row != &c.head; row = row->U) {
        for (DLXNode *node = row->L; node != row; node = node->L) {
            dlxCols[node->colIndex].size++;
            node->U->D = node;
            node->D->U = node;
        }
    }
    // Re-link column header
    c.head.R->L = &c.head;
    c.head.L->R = &c.head;
}

// Minimum remaining values
static Column &chooseColumn() {
    int bestSize = INT_MAX;
    Column *best = nullptr;
    for (DLXNode *cNode = root.R; cNode != &root; cNode = cNode->R) {
        Column &col = dlxCols[cNode->colIndex];
        if (col.size < bestSize) {
            bestSize = col.size;
            best = &col;
            if (bestSize <= 1) break;
        }
    }
    return *best;
}

static bool searchDLX(int depth) {
    if (root.R == &root) {
        // No columns left => solution found
        return true;
    }
    Column &col = chooseColumn();
    if (col.size == 0) return false;
    cover(col);

    for (DLXNode *row = col.head.D; row != &col.head; row = row->D) {
        solutionRows.push_back(row->rowIndex);
        // Cover columns used by this row
        for (DLXNode *node = row->R; node != row; node = node->R) {
            cover(dlxCols[node->colIndex]);
        }
        if (searchDLX(depth + 1)) {
            return true;
        }
        solutionRows.pop_back();
        // Uncover columns
        for (DLXNode *node = row->L; node != row; node = node->L) {
            uncover(dlxCols[node->colIndex]);
        }
    }
    uncover(col);
    return false;
}

static void buildDLX(const vector<array<int,5>> &rowDefs) {
    // Reset root
    root.L = root.R = &root;
    root.U = root.D = &root;
    root.colIndex = -1;
    root.rowIndex = -1;

    // Init column headers
    for (int c = 0; c < DLX_COLS; c++) {
        dlxCols[c].head.L = dlxCols[c].head.R = &dlxCols[c].head;
        dlxCols[c].head.U = dlxCols[c].head.D = &dlxCols[c].head;
        dlxCols[c].head.colIndex = c;
        dlxCols[c].head.rowIndex = -1;
        dlxCols[c].size = 0;
    }

    // Link columns horizontally
    for (int c = 0; c < DLX_COLS; c++) {
        linkLR(root.L, &dlxCols[c].head);
        linkLR(&dlxCols[c].head, &root);
    }

    nodeCount = 0;

    // Insert rows
    for (auto &rd : rowDefs) {
        int rowIdx = rd[0];
        int cA = rd[1], cB = rd[2], cC = rd[3], cD = rd[4];
        DLXNode *rowNodes[4];

        int colIdx[4] = {cA, cB, cC, cD};
        for (int i = 0; i < 4; i++) {
            DLXNode *node = &nodes[nodeCount++];
            node->colIndex = colIdx[i];
            node->rowIndex = rowIdx;

            // Insert in column colIdx[i]
            Column &col = dlxCols[colIdx[i]];
            node->U = col.head.U;
            node->D = &col.head;
            col.head.U->D = node;
            col.head.U = node;
            col.size++;

            if (i == 0) {
                node->L = node;
                node->R = node;
                rowNodes[i] = node;
            } else {
                node->L = rowNodes[i - 1];
                node->R = rowNodes[i - 1]->R;
                rowNodes[i - 1]->R->L = node;
                rowNodes[i - 1]->R = node;
                rowNodes[i] = node;
            }
        }
    }
}

static void fillSolution(int board[N][N]) {
    for (int rowIdx : solutionRows) {
        int d = rowIdx % 9;
        int tmp = rowIdx / 9;
        int c = tmp % 9;
        int r = tmp / 9;
        board[r][c] = d + 1;
    }
}

bool solveSudokuDLX(int board[N][N]) {
    vector<array<int,5>> rowDefs;
    rowDefs.reserve(9 * N * N);

    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) {
            int val = board[r][c];
            if (val != 0) {
                int d = val - 1;
                int rowIdx = encodeRowIndex(r, c, d);
                int col[4];
                candidateToCols(r, c, d, col);
                rowDefs.push_back({rowIdx, col[0], col[1], col[2], col[3]});
            } else {
                for (int d = 0; d < 9; d++) {
                    int rowIdx = encodeRowIndex(r, c, d);
                    int col[4];
                    candidateToCols(r, c, d, col);
                    rowDefs.push_back({rowIdx, col[0], col[1], col[2], col[3]});
                }
            }
        }
    }

    buildDLX(rowDefs);
    solutionRows.clear();

    if (searchDLX(0)) {
        fillSolution(board);
        return true;
    }
    return false;
}

// ------------------------------------------------------------------
//  Ncurses-based TUI
// ------------------------------------------------------------------

static const int START_ROW = 2; // where board starts
static const int START_COL = 2;

int main() {
    // 9x9 board
    static int board[N][N];
    memset(board, 0, sizeof(board));

    // Initialize curses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, true);

    auto printBoard = [&](int highlightR = -1, int highlightC = -1) {
        clear();
        mvprintw(0, 0, "Sudoku TUI (ncurses)");
        mvprintw(1, 0, "Use arrow keys to move, 1..9 to set, 0 or '.' to clear, 'S' to solve, 'Q' to quit.");

        for (int r = 0; r < N; r++) {
            if (r > 0 && r % 3 == 0) {
                mvprintw(START_ROW + 2*r - 1, START_COL, "------+-------+------");
            }
            for (int c = 0; c < N; c++) {
                if (c > 0 && c % 3 == 0) {
                    mvprintw(START_ROW + 2*r, START_COL + 2*c - 1, "| ");
                }
                int val = board[r][c];
                if (r == highlightR && c == highlightC) {
                    attron(A_REVERSE);
                }
                if (val == 0) {
                    mvprintw(START_ROW + 2*r, START_COL + 2*c, ".");
                } else {
                    mvprintw(START_ROW + 2*r, START_COL + 2*c, "%d", val);
                }
                if (r == highlightR && c == highlightC) {
                    attroff(A_REVERSE);
                }
            }
        }
        refresh();
    };

    int curR = 0, curC = 0;
    printBoard(curR, curC);

    while (true) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break; // quit
        } else if (ch == 's' || ch == 'S') {
            bool solved = solveSudokuDLX(board);
            printBoard(curR, curC);
            if (!solved) {
                mvprintw(START_ROW + 2*N + 1, START_COL, "No solution found!");
            } else {
                mvprintw(START_ROW + 2*N + 1, START_COL, "Puzzle solved!");
            }
            refresh();
        } else if (ch == KEY_UP) {
            if (curR > 0) curR--;
            printBoard(curR, curC);
        } else if (ch == KEY_DOWN) {
            if (curR < 8) curR++;
            printBoard(curR, curC);
        } else if (ch == KEY_LEFT) {
            if (curC > 0) curC--;
            printBoard(curR, curC);
        } else if (ch == KEY_RIGHT) {
            if (curC < 8) curC++;
            printBoard(curR, curC);
        } else if ((ch >= '1' && ch <= '9')) {
            board[curR][curC] = ch - '0';
            printBoard(curR, curC);
        } else if (ch == '0' || ch == '.') {
            board[curR][curC] = 0;
            printBoard(curR, curC);
        }
    }

    endwin();
    return 0;
}
