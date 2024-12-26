type Board = [[u8; 9]; 9];

fn main() {
    // Example Sudoku puzzle
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
        println!("No solution found for the given Sudoku puzzle.");
    }
}

/// Solve Sudoku using backtracking with bitmasks.
/// Returns `true` if solved, else `false`.
fn solve_sudoku(board: &mut Board) -> bool {
    // 1) Prepare bitmasks for rows, cols, and boxes to track used digits.
    //    For each row/col/box, if a digit n is used, we set the nth bit to 1.
    let mut row_used = [0u16; 9]; // row_used[r] holds used digits in row r
    let mut col_used = [0u16; 9]; // col_used[c] holds used digits in col c
    let mut box_used = [0u16; 9]; // box_used[b] holds used digits in box b

    // 2) Pre-fill bitmask data from the board
    let mut empty_positions = Vec::new(); // positions of all empty cells
    for r in 0..9 {
        for c in 0..9 {
            let val = board[r][c];
            if val == 0 {
                empty_positions.push((r, c));
            } else {
                // Convert val to zero-based index for bitmask (digit 1..9 -> index 0..8)
                let bit = 1 << (val - 1);
                let box_index = (r / 3) * 3 + (c / 3);
                row_used[r] |= bit;
                col_used[c] |= bit;
                box_used[box_index] |= bit;
            }
        }
    }

    // 3) Start backtracking
    backtrack(
        board,
        0,
        &empty_positions,
        &mut row_used,
        &mut col_used,
        &mut box_used,
    )
}

/// Backtracking function filling empty positions one by one.
fn backtrack(
    board: &mut Board,
    idx: usize,
    empty_positions: &[(usize, usize)],
    row_used: &mut [u16; 9],
    col_used: &mut [u16; 9],
    box_used: &mut [u16; 9],
) -> bool {
    // If we've placed digits in all empty positions, we are done
    if idx == empty_positions.len() {
        return true;
    }

    let (r, c) = empty_positions[idx];
    let b = (r / 3) * 3 + (c / 3);

    // Try digits 1..9
    for digit in 1..=9 {
        let bit = 1 << (digit - 1);

        // Check if digit is free in row, col, and box
        if (row_used[r] & bit) == 0 && (col_used[c] & bit) == 0 && (box_used[b] & bit) == 0 {
            // Place the digit
            board[r][c] = digit;
            // Mark bit as used
            row_used[r] |= bit;
            col_used[c] |= bit;
            box_used[b] |= bit;

            // Recurse
            if backtrack(
                board,
                idx + 1,
                empty_positions,
                row_used,
                col_used,
                box_used,
            ) {
                return true;
            }

            // Undo the choice (backtrack)
            board[r][c] = 0;
            row_used[r] &= !bit;
            col_used[c] &= !bit;
            box_used[b] &= !bit;
        }
    }
    false
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
