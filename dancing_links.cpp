#include <bits/stdc++.h>
using namespace std;

// Dimensions
static const int N = 9;             // 9x9 Sudoku
static const int COLS = 4 * N * N;  // 4 constraints per each of 81 cells => 324 columns

// Each DLX node has up/down/left/right pointers + rowIndex + colIndex.
struct DLXNode {
    DLXNode* L;
    DLXNode* R;
    DLXNode* U;
    DLXNode* D;
    int colIndex;
    int rowIndex;
};

// Each column has its own header node and a 'size' = number of rows in that column.
struct Column {
    DLXNode head;
    int size;
};

// We'll store *all* nodes in a static array to avoid new/delete overhead.
static const int MAX_NODES = 9 * 9 * 9 * 4 + 5000; // A safe upper bound
static DLXNode nodes[MAX_NODES];
static int nodeCount = 0;    // index in nodes[] pool

// 324 columns for Sudoku
static Column cols[COLS];
static DLXNode root;         // Root node of the Dancing Links structure

// We'll collect the final solution row indices here.
static vector<int> solutionRows;

// Forward declarations
static bool searchDLX(int depth);

// ------------------------------------------------------------------
// Helper functions
// ------------------------------------------------------------------

// Convert (row, col) => index of the 3x3 box
inline int boxIndex(int r, int c) {
    return (r / 3) * 3 + (c / 3); // each 3x3 block
}

// Encode (r, c, d) into a single int [0..728]
inline int encodeRowIndex(int r, int c, int d) {
    // r in [0..8], c in [0..8], d in [0..8]
    // rowIndex = r*81 + c*9 + d
    return (r * 81) + (c * 9) + d;
}

// For each candidate (r, c, d), it satisfies 4 constraints => 4 column indexes
// 1) Cell occupancy:    r*9 + c           (which cell is used)
// 2) Row-digit:         81 + (r*9 + d)    (row r, digit d)
// 3) Col-digit:         162 + (c*9 + d)   (col c, digit d)
// 4) Box-digit:         243 + (box*9 + d) (boxIndex(r,c), digit d)
inline void candidateToCols(int r, int c, int d, int outCols[4]) {
    outCols[0] = r * 9 + c;                 // cell
    outCols[1] = 81 + (r * 9 + d);          // row-digit
    outCols[2] = 162 + (c * 9 + d);         // col-digit
    outCols[3] = 243 + (boxIndex(r, c) * 9 + d); // box-digit
}

// ------------------------------------------------------------------
// Dancing Links operations
// ------------------------------------------------------------------

// Link two nodes horizontally
inline void linkLR(DLXNode* left, DLXNode* right) {
    left->R = right;
    right->L = left;
}

// Link two nodes vertically
inline void linkUD(DLXNode* up, DLXNode* down) {
    up->D = down;
    down->U = up;
}

// "Cover" a column => remove it from the matrix
static void cover(Column &c) {
    // Remove the column header from the root’s LR list
    c.head.R->L = c.head.L;
    c.head.L->R = c.head.R;

    // For each row in this column
    for (DLXNode* rowNode = c.head.D; rowNode != &c.head; rowNode = rowNode->D) {
        // Remove the rowNode from other columns in its row
        for (DLXNode* node = rowNode->R; node != rowNode; node = node->R) {
            node->U->D = node->D;
            node->D->U = node->U;
            cols[node->colIndex].size--;
        }
    }
}

// "Uncover" a column => restore it to the matrix
static void uncover(Column &c) {
    // Reinsert each row from bottom to top
    for (DLXNode* rowNode = c.head.U; rowNode != &c.head; rowNode = rowNode->U) {
        // Reinsert the rowNode into the other columns
        for (DLXNode* node = rowNode->L; node != rowNode; node = node->L) {
            cols[node->colIndex].size++;
            node->U->D = node;
            node->D->U = node;
        }
    }
    // Re-link this column’s header
    c.head.R->L = &c.head;
    c.head.L->R = &c.head;
}

// Choose the column with the smallest size => "MRV" heuristic
static Column& chooseColumn() {
    int bestSize = INT_MAX;
    Column* best = nullptr;

    // Traverse columns from root.R to root
    for (DLXNode* cNode = root.R; cNode != &root; cNode = cNode->R) {
        Column &col = cols[cNode->colIndex];
        if (col.size < bestSize) {
            bestSize = col.size;
            best = &col;
            if (bestSize <= 1) break; // can't do better than 1
        }
    }
    return *best;
}

// Algorithm X search
static bool searchDLX(int depth) {
    // If there are no columns left, we found a solution
    if (root.R == &root) {
        return true;
    }
    // Choose a column with fewest rows
    Column &col = chooseColumn();
    if (col.size == 0) {
        // No possible row => failure
        return false;
    }
    // Cover this column
    cover(col);

    // Try each row in col
    for (DLXNode* rowNode = col.head.D; rowNode != &col.head; rowNode = rowNode->D) {
        // rowNode->rowIndex is an encoded (r, c, d)
        solutionRows.push_back(rowNode->rowIndex);

        // Cover all columns in this row
        for (DLXNode* node = rowNode->R; node != rowNode; node = node->R) {
            cover(cols[node->colIndex]);
        }

        // Recurse
        if (searchDLX(depth + 1)) {
            return true;
        }

        // Backtrack
        solutionRows.pop_back();
        // Uncover columns in reverse order
        for (DLXNode* node = rowNode->L; node != rowNode; node = node->L) {
            uncover(cols[node->colIndex]);
        }
    }
    // Uncover this column
    uncover(col);
    return false;
}

// ------------------------------------------------------------------
// Build the matrix
// ------------------------------------------------------------------

// We pass a list of row definitions: for each valid candidate we have
// { rowIndex, colA, colB, colC, colD }
// rowIndex = encodeRowIndex(r,c,d)
// colA..colD = the 4 constraints (0..323)
static void buildDLX(const vector<array<int,5>> &rowDefs) {
    // Clear the root
    root.L = root.R = &root;
    root.U = root.D = &root;
    root.colIndex = -1;
    root.rowIndex = -1;

    // Initialize column headers
    for (int c = 0; c < COLS; c++) {
        cols[c].head.L = cols[c].head.R = &cols[c].head;
        cols[c].head.U = cols[c].head.D = &cols[c].head;
        cols[c].head.colIndex = c;
        cols[c].head.rowIndex = -1;
        cols[c].size = 0;
    }

    // Link column headers horizontally into root
    for (int c = 0; c < COLS; c++) {
        // Insert to the left of root
        cols[c].head.R = &root;
        cols[c].head.L = root.L;
        root.L->R = &cols[c].head;
        root.L = &cols[c].head;
    }

    // Reset node pool
    nodeCount = 0;

    // Insert each row
    for (auto &rd : rowDefs) {
        int rowIndex = rd[0];
        int colArr[4] = {rd[1], rd[2], rd[3], rd[4]};

        // We create 4 nodes for this row, linking them horizontally
        DLXNode* rowNodes[4];
        for (int i = 0; i < 4; i++) {
            int cIndex = colArr[i];
            DLXNode* node = &nodes[nodeCount++];
            node->colIndex = cIndex;
            node->rowIndex = rowIndex;

            // Insert vertically into column cIndex
            Column &col = cols[cIndex];
            node->U = col.head.U;
            node->D = &col.head;
            col.head.U->D = node;
            col.head.U = node;
            col.size++;

            // Link horizontally
            if (i == 0) {
                // First node in this row
                node->L = node;
                node->R = node;
                rowNodes[i] = node;
            } else {
                // Link to the left
                node->L = rowNodes[i - 1];
                node->R = rowNodes[i - 1]->R;
                rowNodes[i - 1]->R->L = node;
                rowNodes[i - 1]->R = node;
                rowNodes[i] = node;
            }
        }
    }
}

// After solving, fill the final board using the chosen row indices
static void fillSolution(int board[N][N]) {
    for (int rowIndex : solutionRows) {
        int d = rowIndex % 9;
        int tmp = rowIndex / 9;
        int c = tmp % 9;
        int r = tmp / 9;
        board[r][c] = d + 1; // digits are 1..9
    }
}

// Solve Sudoku with DLX
bool solveSudokuDLX(int board[N][N]) {
    // 1) Build rowDefs
    // We need up to 9 candidates per empty cell, or 1 if cell is given
    vector<array<int,5>> rowDefs; 
    rowDefs.reserve(9 * N * N);  // up to 729

    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) {
            int given = board[r][c];
            if (given != 0) {
                // There's exactly one possible digit
                int d = given - 1;
                int rowIdx = encodeRowIndex(r, c, d);
                int colId[4];
                candidateToCols(r, c, d, colId);
                rowDefs.push_back({rowIdx, colId[0], colId[1], colId[2], colId[3]});
            } else {
                // Cell empty => try all 9 digits
                for (int d = 0; d < 9; d++) {
                    int rowIdx = encodeRowIndex(r, c, d);
                    int colId[4];
                    candidateToCols(r, c, d, colId);
                    rowDefs.push_back({rowIdx, colId[0], colId[1], colId[2], colId[3]});
                }
            }
        }
    }

    // 2) Build the DLX structure
    buildDLX(rowDefs);

    // 3) Clear solutionRows from any previous run
    solutionRows.clear();

    // 4) Search
    if (searchDLX(0)) {
        // Found solution => fill board
        fillSolution(board);
        return true;
    }
    return false;
}

// Utility function to print Sudoku board
static void printBoard(int board[N][N]) {
    for (int r = 0; r < N; r++) {
        if (r > 0 && r % 3 == 0) {
            cout << "------+-------+------\n";
        }
        for (int c = 0; c < N; c++) {
            if (c > 0 && c % 3 == 0) cout << "| ";
            if (board[r][c] == 0) cout << ". ";
            else cout << board[r][c] << " ";
        }
        cout << "\n";
    }
}

// ------------------------------------------------------------------
// Main with your puzzle
// ------------------------------------------------------------------

int main() {
    // Puzzle from your example (bitmask solver finishes in near-zero time):
    //  8 . . | 5 3 2 | 7 . .
    //  6 . 2 | . 9 8 | . . 4
    //  . . . | . . 6 | . . .
    //  ------+-------+------
    //  4 . . | . 1 . | . . .
    //  . 5 6 | . 2 . | 4 8 .
    //  . . . | . 6 . | . . 2
    //  ------+-------+------
    //  . . . | 9 . . | . . .
    //  2 . . | 6 8 . | 9 . 3
    //  . . 4 | 2 7 1 | . . 8
    // Let's encode that as a board, where 0 = empty:
    int board[N][N] = {
        {8, 0, 0, 5, 3, 2, 7, 0, 0},
        {6, 0, 2, 0, 9, 8, 0, 0, 4},
        {0, 0, 0, 0, 0, 6, 0, 0, 0},

        {4, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 5, 6, 0, 2, 0, 4, 8, 0},
        {0, 0, 0, 0, 6, 0, 0, 0, 2},

        {0, 0, 0, 9, 0, 0, 0, 0, 0},
        {2, 0, 0, 6, 8, 0, 9, 0, 3},
        {0, 0, 4, 2, 7, 1, 0, 0, 8}
    };

    cout << "Initial Sudoku Puzzle:\n";
    printBoard(board);

    bool solved = solveSudokuDLX(board);
    if (solved) {
        cout << "\nSolved Sudoku Puzzle:\n";
        printBoard(board);
    } else {
        cout << "\nNo solution found.\n";
    }

    return 0;
}
