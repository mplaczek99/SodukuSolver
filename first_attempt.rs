use std::io;

/// A 9×9 Sudoku board; 0 represents an empty cell.
type Board = [[u8; 9]; 9];

fn main() {
    // Example puzzle: 0 means empty.
    // This puzzle should be solvable.
    let mut board: Board = [
        [5, 3, 0, 0, 7, 0, 0, 0, 0],
        [6, 0, 0, 1, 9, 5, 0, 0, 0],
        [0, 9, 8, 0, 0, 0, 0, 6, 0],
        [8, 0, 0, 0, 6, 0, 0, 0, 3],
        [4, 0, 0, 8, 0, 3, 0, 0, 1],
        [7, 0, 0, 0, 2, 0, 0, 0, 6],
        [0, 6, 0, 0, 0, 0, 2, 8, 0],
        [0, 0, 0, 4, 1, 9, 0, 0, 5],
        [0, 0, 0, 0, 8, 0, 0, 7, 9],
    ];

    println!("Initial Sudoku Puzzle:");
    print_board(&board);

    if solve_sudoku(&mut board) {
        println!("\nSolved Sudoku Puzzle:");
        print_board(&board);
    } else {
        println!("\nNo solution found for the given Sudoku puzzle.");
    }
}

/// Solves the Sudoku puzzle in-place using backtracking.
/// Returns `true` if a solution is found, otherwise `false`.
fn solve_sudoku(board: &mut Board) -> bool {
    // 1. Find an empty location on the board. If none, we are done.
    let empty_pos = find_empty_position(board);
    if empty_pos.is_none() {
        return true; // No empty cells, puzzle solved.
    }
    let (row, col) = empty_pos.unwrap();

    // 2. Try digits 1 through 9 in the empty cell.
    for num in 1..=9 {
        if is_valid(board, row, col, num) {
            board[row][col] = num;

            // 3. Recur to see if this choice leads to a solution.
            if solve_sudoku(board) {
                return true;
            }

            // 4. Backtrack if not a solution.
            board[row][col] = 0;
        }
    }

    // If no valid number works here, return false to backtrack.
    false
}

/// Finds an unassigned (empty) cell in the board. Returns `Some((row, col))` if found,
/// or `None` if there is no empty cell.
fn find_empty_position(board: &Board) -> Option<(usize, usize)> {
    for row in 0..9 {
        for col in 0..9 {
            if board[row][col] == 0 {
                return Some((row, col));
            }
        }
    }
    None
}

/// Checks if placing `num` at position `(row, col)` is valid
/// according to the Sudoku rules (row, column, and 3×3 box).
fn is_valid(board: &Board, row: usize, col: usize, num: u8) -> bool {
    // Check row
    for x in 0..9 {
        if board[row][x] == num {
            return false;
        }
    }

    // Check column
    for x in 0..9 {
        if board[x][col] == num {
            return false;
        }
    }

    // Check 3×3 subgrid
    let subgrid_row = (row / 3) * 3;
    let subgrid_col = (col / 3) * 3;
    for r in subgrid_row..subgrid_row + 3 {
        for c in subgrid_col..subgrid_col + 3 {
            if board[r][c] == num {
                return false;
            }
        }
    }

    true
}

/// Prints the Sudoku board to the console.
fn print_board(board: &Board) {
    for (i, row) in board.iter().enumerate() {
        if i % 3 == 0 && i != 0 {
            println!("------+-------+------");
        }
        for (j, &val) in row.iter().enumerate() {
            if j % 3 == 0 && j != 0 {
                print!("| ");
            }
            print!("{} ", if val == 0 { '.' } else { (val + b'0') as char });
        }
        println!();
    }
}
