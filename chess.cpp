// Licensed with GPL 3.0
#include <cctype>
#include <cmath>
#include <iostream>
#include <ostream>
#include <string>

bool whitesMove = true;
int halfMove = 0;
int fiftyMoveCounter = 0;
bool running = true;

// It would segfault without this if invalid input was entered. This is the only
// part of code I took from GPT but it still isn't verbatim. Checks if the move
// string is 2 in length, the first character is between a-h and the second
// character is between 1-8
bool itsSafe(const std::string &move) {
  return move.size() == 2 && std::tolower(move[0]) >= 'a' &&
         std::tolower(move[0]) <= 'h' && move[1] >= '1' && move[1] <= '8';
}

// The piece class used for the board matrix. Provides a constructor for
// creating and moving pieces.
class Piece {
public:
  int rank;
  int file;
  char type;
  bool isWhite;
  bool inWhitesCheck = false;
  bool inBlacksCheck = false;
  bool hasntMoved = false;
  int enPassant;
  Piece(int rank, int file, char type, bool isWhite, bool hasntMoved) {
    this->rank = rank;
    this->file = file;
    this->type = type;
    this->isWhite = isWhite;
    this->hasntMoved = hasntMoved;
  }
  // Empty square
  Piece() { this->type = '.'; };
};

// Overloaded functions, used to convert algebraic notation to matrix indexes
int fileConv(char file) { return file - 'a'; };

char fileConv(int file) { return 'a' + file; };

int rankConv(char rank) { return '8' - rank; };

char rankConv(int rank) { return 56 - rank; }

Piece board[8][8];
Piece backupboard[8][8];

// Copies the board to a backup array to rollback if an impossible move is made
void transaction() {
  for (int i = 0; i <= 7; i++) {
    for (int j = 0; j <= 7; j++) {
      backupboard[i][j] = board[i][j];
    }
  }
}

// Rollsback the board to the backup board
void rollback() {
  for (int i = 0; i <= 7; i++) {
    for (int j = 0; j <= 7; j++) {
      board[i][j] = backupboard[i][j];
    }
  }
}

int kingRank(bool white) {
  for (int i = 0; i <= 7; i++) {
    for (int j = 0; j <= 7; j++) {
      if (std::tolower(board[i][j].type) == 'k' &&
          board[i][j].isWhite == white) {
        return i;
      }
    }
  }
  std::cout << "Fatal error: King doesn't exist" << std::endl;
  return 0;
}

int kingFile(bool white) {
  for (int i = 0; i <= 7; i++) {
    for (int j = 0; j <= 7; j++) {
      if (std::tolower(board[i][j].type) == 'k' &&
          board[i][j].isWhite == white) {
        return j;
      }
    }
  }
  std::cout << "Fatal error: King doesn't exist" << std::endl;
  return 0;
}
// Transfers all properties of the piece from the source square to the target
// square, except the hasntMoved boolean value
void updateCheckMap();

void makeMove(int fromRank, int fromFile, int toRank, int toFile) {
  transaction();
  board[toRank][toFile] = board[fromRank][fromFile];
  board[fromRank][fromFile] = Piece();
  board[toRank][toFile].rank = toRank;
  board[toRank][toFile].file = toFile;
  board[toRank][toFile].hasntMoved = false;
  updateCheckMap();
  if (whitesMove) {
    if (board[kingRank(true)][kingFile(true)].inBlacksCheck) {
      rollback();
      // std::cout << "Impossible move." << std::endl;
      return;
    }
  } else {
    if (board[kingRank(false)][kingFile(false)].inWhitesCheck) {
      rollback();
      // std::cout << "Impossible move." << std::endl;
      return;
    }
  }
  // Pawn promotion check
  if (std::tolower(board[toRank][toFile].type) == 'p' &&
      (toRank == 0 || toRank == 7)) {
    std::cout << "Which piece do you want to promote to? (r, n, q, b)" << '\n';
    char promoteTo;
    do {
      std::cin >> promoteTo;
      promoteTo = std::tolower(promoteTo);
    } while (promoteTo != 'q' && promoteTo != 'b' && promoteTo != 'r' &&
             promoteTo != 'n');
    board[toRank][toFile].type =
        board[toRank][toFile].isWhite ? std::toupper(promoteTo) : promoteTo;
  }
  // Increment counter
  whitesMove = !whitesMove;
  halfMove++;
  if (std::tolower(board[fromRank][fromFile].type) == 'p' ||
      board[toRank][toFile].type != '.') {
    fiftyMoveCounter = 0;
  } else {
    fiftyMoveCounter++;
  }
  // std::cout << "makeMove triggered" << std::endl; // DEBUG
}

// The brains of the code, checks all moves for validity
void checkMove(int fromRank, int fromFile, int toRank, int toFile) {
  // Couple general checks that can be used for the movement for any piece
  if ((fromRank == toRank) && (fromFile == toFile)) {
    return;
  }
  if ((board[toRank][toFile].isWhite == board[fromRank][fromFile].isWhite) &&
      (board[toRank][toFile].type != '.')) {
    return;
  }
  // std::cout << "Friendly fire capture check passed" << std::endl;
  if (fromRank < 0 || fromRank > 7 || fromFile < 0 || fromFile > 7 ||
      toRank < 0 || toRank > 7 || toFile < 0 || toFile > 7) {
    return;
  }
  if (board[fromRank][fromFile].isWhite != whitesMove) {
    return;
  }
  // std:cout << "checkMove general move rules passed" << std::endl; // DEBUG
  switch (std::tolower(board[fromRank][fromFile].type)) {
  // Rook check: Checks if the target square matches the rank or file with the
  // source square, then iterates over the squares in between to check if they
  // are empty
  case 'r':
    if (fromRank == toRank) {
      if (toFile > fromFile) {
        bool foundPiece = false;
        for (int i = fromFile + 1; i < toFile; i++) {
          if (board[fromRank][i].type != '.') {
            foundPiece = true;
            break;
          }
        }
        if (!foundPiece) {
          makeMove(fromRank, fromFile, toRank, toFile);
        }
      } else {
        bool foundPiece = false;
        for (int i = toFile + 1; i < fromFile; i++) {
          if (board[fromRank][i].type != '.') {
            foundPiece = true;
            break;
          }
        }
        if (!foundPiece) {
          makeMove(fromRank, fromFile, toRank, toFile);
        }
      }
    }
    if (fromFile == toFile) {
      if (toRank > fromRank) {
        bool foundPiece = false;
        for (int i = fromRank + 1; i < toRank; i++) {
          if (board[i][fromFile].type != '.') {
            foundPiece = true;
            break;
          }
        }
        if (!foundPiece) {
          makeMove(fromRank, fromFile, toRank, toFile);
        }
      } else {
        bool foundPiece = false;
        for (int i = toRank + 1; i < fromRank; i++) {
          if (board[i][fromFile].type != '.') {
            foundPiece = true;
            break;
          }
        }
        if (!foundPiece) {
          makeMove(fromRank, fromFile, toRank, toFile);
        }
      }
    }
    break;
  // Bishop check: First determines if the x and y distance to the target square
  // is the same(in the diagonal), then assigns multipliers for the direction
  // the bishop is heading. Using multipliers, it checks for obstructions in
  // between without repeating code
  case 'b':
    if (std::abs(fromRank - toRank) == std::abs(fromFile - toFile)) {
      int rMult = 1;
      int fMult = 1;
      if (fromRank > toRank) {
        rMult = -1;
      }
      if (fromFile > toFile) {
        fMult = -1;
      }
      bool foundPiece = false;
      for (int i = 1; i < std::abs(fromFile - toFile); i++) {
        if (board[fromRank + i * rMult][fromFile + i * fMult].type != '.') {
          foundPiece = true;
          break;
        }
      }
      if (!foundPiece) {
        makeMove(fromRank, fromFile, toRank, toFile);
      }
    }
    break;
  case 'q':
    // This is just the rook and bishop combined, could have been a function but
    // this is simpler
    // The bishop part:
    if (std::abs(fromRank - toRank) == std::abs(fromFile - toFile)) {
      int rMult = 1;
      int fMult = 1;
      if (fromRank > toRank) {
        rMult = -1;
      }
      if (fromFile > toFile) {
        fMult = -1;
      }
      bool foundPiece = false;
      for (int i = 1; i < std::abs(fromFile - toFile); i++) {
        if (board[fromRank + i * rMult][fromFile + i * fMult].type != '.') {
          foundPiece = true;
          break;
        }
      }
      if (!foundPiece) {
        makeMove(fromRank, fromFile, toRank, toFile);
      }
    }
    // The rook part:
    if (fromRank == toRank) {
      if (toFile > fromFile) {
        bool foundPiece = false;
        for (int i = fromFile + 1; i < toFile; i++) {
          if (board[fromRank][i].type != '.') {
            foundPiece = true;
            break;
          }
        }
        if (!foundPiece) {
          makeMove(fromRank, fromFile, toRank, toFile);
        }
      } else {
        bool foundPiece = false;
        for (int i = toFile + 1; i < fromFile; i++) {
          if (board[fromRank][i].type != '.') {
            foundPiece = true;
            break;
          }
        }
        if (!foundPiece) {
          makeMove(fromRank, fromFile, toRank, toFile);
        }
      }
    }
    if (fromFile == toFile) {
      if (toRank > fromRank) {
        bool foundPiece = false;
        for (int i = fromRank + 1; i < toRank; i++) {
          if (board[i][fromFile].type != '.') {
            foundPiece = true;
            break;
          }
        }
        if (!foundPiece) {
          makeMove(fromRank, fromFile, toRank, toFile);
        }
      } else {
        bool foundPiece = false;
        for (int i = toRank + 1; i < fromRank; i++) {
          if (board[i][fromFile].type != '.') {
            foundPiece = true;
            break;
          }
        }
        if (!foundPiece) {
          makeMove(fromRank, fromFile, toRank, toFile);
        }
      }
    }
    break;
  // The knight is pretty simple. Just checks if the x distance is 2 and the y
  // distance is 1 or vice versa. The general move check handles out of bounds
  // move prevention
  case 'n':
    if ((std::abs(fromRank - toRank) == 2 &&
         std::abs(fromFile - toFile) == 1) ||
        (std::abs(fromRank - toRank) == 1 &&
         std::abs(fromFile - toFile) == 2)) {
      makeMove(fromRank, fromFile, toRank, toFile);
    }
    break;
  // The king has no castling currently, nor illegal move preventions. Just
  // checks if distance to the square in either the x or y direction is equal or
  // less than 1. The general check before the switch handles if both distances
  // are 0
  // Added castling and illegal move prevention, only thing left to make this
  // script complete chess is stalemate and checkmate
  case 'k':
    if (std::abs(fromRank - toRank) <= 1 && std::abs(fromFile - toFile) <= 1) {
      makeMove(fromRank, fromFile, toRank, toFile);
    }
    if (board[fromRank][fromFile].hasntMoved && toFile == 6 &&
        board[fromRank][7].hasntMoved &&
        std::tolower(board[fromRank][7].type) == 'r' &&
        board[fromRank][5].type == '.' && board[fromRank][6].type == '.' &&
        fromRank == toRank) {
      if (board[fromRank][fromFile].isWhite) {
        if (!board[fromRank][fromFile].inBlacksCheck &&
            !board[fromRank][5].inBlacksCheck &&
            !board[fromRank][6].inBlacksCheck) {
          makeMove(fromRank, fromFile, toRank, toFile);
          makeMove(7, 7, 7, 5);
          halfMove--;
          whitesMove = !whitesMove;
        }
      } else {
        if (!board[fromRank][fromFile].inWhitesCheck &&
            !board[fromRank][5].inWhitesCheck &&
            !board[fromRank][6].inWhitesCheck) {
          makeMove(fromRank, fromFile, toRank, toFile);
          makeMove(0, 7, 0, 5);
          halfMove--;
          whitesMove = !whitesMove;
        }
      }
    }
    if (toFile == 2 && toRank == fromRank &&
        board[fromRank][fromFile].hasntMoved && board[fromRank][0].hasntMoved &&
        std::tolower(board[fromRank][0].type) == 'r' &&
        board[fromRank][3].type == '.' && board[fromRank][2].type == '.' &&
        board[fromRank][1].type == '.') {
      if (board[fromRank][fromFile].isWhite) {
        if (!board[7][4].inBlacksCheck && !board[7][3].inBlacksCheck &&
            !board[7][2].inBlacksCheck) {
          makeMove(fromRank, fromFile, toRank, toFile);
          makeMove(7, 0, 7, 3);
          halfMove--;
          whitesMove = !whitesMove;
        }
      } else {
        if (!board[0][4].inWhitesCheck && !board[0][3].inWhitesCheck &&
            !board[0][2].inWhitesCheck) {
          makeMove(fromRank, fromFile, toRank, toFile);
          makeMove(0, 0, 0, 3);
          halfMove--;
          whitesMove = !whitesMove;
        }
      }
    }
    break;
  // The pawn moves forward if the square is empty, forward determined with
  // the color multiplier. Can move 2 squares if it hasn't moved yet, checking
  // if the square in between is empty. The first move assigns the current
  // halfMove to the in between square for en passant. Capturing just checks
  // if there is a piece of the opponent (or an en passant square with the
  // current half move) in the forward diagonals
  case 'p':
    int colorMult = 1;
    if (!board[fromRank][fromFile].isWhite) {
      colorMult = -1;
    }
    if (fromFile == toFile) {
      if (toRank + 1 * colorMult == fromRank &&
          board[toRank][toFile].type == '.') {
        makeMove(fromRank, fromFile, toRank, toFile);
      }
      if (toRank + 2 * colorMult == fromRank &&
          board[fromRank][fromFile].hasntMoved &&
          board[toRank][toFile].type == '.' &&
          board[toRank + 1 * colorMult][toFile].type == '.') {
        makeMove(fromRank, fromFile, toRank, toFile);
        board[toRank + 1 * colorMult][toFile].enPassant = halfMove;
      }
    }
    if (std::abs(fromFile - toFile) == 1 && toRank == fromRank - colorMult) {
      if (board[toRank][toFile].type != '.') {
        makeMove(fromRank, fromFile, toRank, toFile);
      }
      if (board[toRank][toFile].enPassant == halfMove) {
        makeMove(fromRank, fromFile, toRank, toFile);
        board[toRank + colorMult][toFile] = Piece();
      }
    }
    break;
  }
}

// Updates the check state of the board, copied mostly from the checkMove
// function

bool anyLegalMoves() {
  Piece tempBoard[8][8];
  int currentHalfMove = halfMove;
  bool move = whitesMove;
  int currentFiftyMoveCounter = fiftyMoveCounter;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (board[i][j].type == '.') {
        continue;
      }
      if (board[i][j].isWhite != whitesMove) {
        continue;
      }
      for (int k = 0; k < 8; k++) {
        for (int l = 0; l < 8; l++) {
          for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
              tempBoard[rank][file] = board[rank][file];
            }
          }
          updateCheckMap();
          checkMove(i, j, k, l);
          bool succeeded = (halfMove != currentHalfMove);
          for (int rank = 0; rank <= 7; rank++) {
            for (int file = 0; file <= 7; file++) {
              board[rank][file] = tempBoard[rank][file];
            }
          }
          halfMove = currentHalfMove;
          whitesMove = move;
          fiftyMoveCounter = currentFiftyMoveCounter;
          if (succeeded) {
            updateCheckMap();
            // std::cout << "anyLegalMoves() caught a legal move with these "
            //"parameters: "; // DEBUG
            // std::cout << i << j << k << l << std::endl;
            return true;
          }
        }
      }
    }
  }
  return false;
}

void assignCheck(int rank, int file, bool white) {
  if (rank <= 7 && rank >= 0 && file <= 7 && file >= 0) {
    if (white) {
      board[rank][file].inWhitesCheck = true;
    } else {
      board[rank][file].inBlacksCheck = true;
    }
  }
}
// I'm so dumb I should've implemented this function before getting to the
// pawn and king. Would've saved like 100 lines of code and potential bugs
void updateCheckMap() {
  for (int rank = 0; rank <= 7; rank++) {
    for (int file = 0; file <= 7; file++) {
      board[rank][file].inBlacksCheck = false;
      board[rank][file].inWhitesCheck = false;
    }
  }
  for (int i = 0; i <= 7; i++) {
    for (int j = 0; j <= 7; j++) {
      int iMult = 1;
      int jMult = 1;
      int colorMult = 1;
      switch (std::tolower(board[i][j].type)) {
      case 'r':
        for (int k = i + 1; k <= 7; k++) {
          if (board[i][j].isWhite) {
            board[k][j].inWhitesCheck = true;
          } else {
            board[k][j].inBlacksCheck = true;
          }
          if (board[k][j].type != '.') {
            break;
          }
        }
        for (int k = i - 1; k >= 0; k--) {
          if (board[i][j].isWhite) {
            board[k][j].inWhitesCheck = true;
          } else {
            board[k][j].inBlacksCheck = true;
          }
          if (board[k][j].type != '.') {
            break;
          }
        }
        for (int k = j + 1; k <= 7; k++) {
          if (board[i][j].isWhite) {
            board[i][k].inWhitesCheck = true;
          } else {
            board[i][k].inBlacksCheck = true;
          }
          if (board[i][k].type != '.') {
            break;
          }
        }
        for (int k = j - 1; k >= 0; k--) {
          if (board[i][j].isWhite) {
            board[i][k].inWhitesCheck = true;
          } else {
            board[i][k].inBlacksCheck = true;
          }
          if (board[i][k].type != '.') {
            break;
          }
        }
        break;
      case 'b':
        // Could've used lambda functions for this instead of doing it four
        // times but I'm not familiar with how to use them
        iMult = 1;
        jMult = 1;
        for (int k = 1; i + k * iMult <= 7 && j + k * jMult <= 7 &&
                        i + k * iMult >= 0 && j + k * jMult >= 0;
             k++) {
          if (board[i][j].isWhite) {
            board[i + k * iMult][j + k * jMult].inWhitesCheck = true;
          } else {
            board[i + k * iMult][j + k * jMult].inBlacksCheck = true;
          }
          if (board[i + k * iMult][j + k * jMult].type != '.') {
            break;
          }
        }
        iMult = -1;
        jMult = 1;
        for (int k = 1; i + k * iMult <= 7 && j + k * jMult <= 7 &&
                        i + k * iMult >= 0 && j + k * jMult >= 0;
             k++) {
          if (board[i][j].isWhite) {
            board[i + k * iMult][j + k * jMult].inWhitesCheck = true;
          } else {
            board[i + k * iMult][j + k * jMult].inBlacksCheck = true;
          }
          if (board[i + k * iMult][j + k * jMult].type != '.') {
            break;
          }
        }
        iMult = 1;
        jMult = -1;
        for (int k = 1; i + k * iMult <= 7 && j + k * jMult <= 7 &&
                        i + k * iMult >= 0 && j + k * jMult >= 0;
             k++) {
          if (board[i][j].isWhite) {
            board[i + k * iMult][j + k * jMult].inWhitesCheck = true;
          } else {
            board[i + k * iMult][j + k * jMult].inBlacksCheck = true;
          }
          if (board[i + k * iMult][j + k * jMult].type != '.') {
            break;
          }
        }
        iMult = -1;
        jMult = -1;
        for (int k = 1; i + k * iMult <= 7 && j + k * jMult <= 7 &&
                        i + k * iMult >= 0 && j + k * jMult >= 0;
             k++) {
          if (board[i][j].isWhite) {
            board[i + k * iMult][j + k * jMult].inWhitesCheck = true;
          } else {
            board[i + k * iMult][j + k * jMult].inBlacksCheck = true;
          }
          if (board[i + k * iMult][j + k * jMult].type != '.') {
            break;
          }
        }
        break;
      case 'q':
        for (int k = i + 1; k <= 7; k++) {
          if (board[i][j].isWhite) {
            board[k][j].inWhitesCheck = true;
          } else {
            board[k][j].inBlacksCheck = true;
          }
          if (board[k][j].type != '.') {
            break;
          }
        }
        for (int k = i - 1; k >= 0; k--) {
          if (board[i][j].isWhite) {
            board[k][j].inWhitesCheck = true;
          } else {
            board[k][j].inBlacksCheck = true;
          }
          if (board[k][j].type != '.') {
            break;
          }
        }
        for (int k = j + 1; k <= 7; k++) {
          if (board[i][j].isWhite) {
            board[i][k].inWhitesCheck = true;
          } else {
            board[i][k].inBlacksCheck = true;
          }
          if (board[i][k].type != '.') {
            break;
          }
        }
        for (int k = j - 1; k >= 0; k--) {
          if (board[i][j].isWhite) {
            board[i][k].inWhitesCheck = true;
          } else {
            board[i][k].inBlacksCheck = true;
          }
          if (board[i][k].type != '.') {
            break;
          }
        }
        iMult = 1;
        jMult = 1;
        for (int k = 1; i + k * iMult <= 7 && j + k * jMult <= 7 &&
                        i + k * iMult >= 0 && j + k * jMult >= 0;
             k++) {
          if (board[i][j].isWhite) {
            board[i + k * iMult][j + k * jMult].inWhitesCheck = true;
          } else {
            board[i + k * iMult][j + k * jMult].inBlacksCheck = true;
          }
          if (board[i + k * iMult][j + k * jMult].type != '.') {
            break;
          }
        }
        iMult = -1;
        jMult = 1;
        for (int k = 1; i + k * iMult <= 7 && j + k * jMult <= 7 &&
                        i + k * iMult >= 0 && j + k * jMult >= 0;
             k++) {
          if (board[i][j].isWhite) {
            board[i + k * iMult][j + k * jMult].inWhitesCheck = true;
          } else {
            board[i + k * iMult][j + k * jMult].inBlacksCheck = true;
          }
          if (board[i + k * iMult][j + k * jMult].type != '.') {
            break;
          }
        }
        iMult = 1;
        jMult = -1;
        for (int k = 1; i + k * iMult <= 7 && j + k * jMult <= 7 &&
                        i + k * iMult >= 0 && j + k * jMult >= 0;
             k++) {
          if (board[i][j].isWhite) {
            board[i + k * iMult][j + k * jMult].inWhitesCheck = true;
          } else {
            board[i + k * iMult][j + k * jMult].inBlacksCheck = true;
          }
          if (board[i + k * iMult][j + k * jMult].type != '.') {
            break;
          }
        }
        iMult = -1;
        jMult = -1;
        for (int k = 1; i + k * iMult <= 7 && j + k * jMult <= 7 &&
                        i + k * iMult >= 0 && j + k * jMult >= 0;
             k++) {
          if (board[i][j].isWhite) {
            board[i + k * iMult][j + k * jMult].inWhitesCheck = true;
          } else {
            board[i + k * iMult][j + k * jMult].inBlacksCheck = true;
          }
          if (board[i + k * iMult][j + k * jMult].type != '.') {
            break;
          }
        }
        break;
      case 'n':
        if (board[i][j].isWhite) {
          if (i + 2 <= 7 && j + 1 <= 7) {
            board[i + 2][j + 1].inWhitesCheck = true;
          }
          if (i + 1 <= 7 && j + 2 <= 7) {
            board[i + 1][j + 2].inWhitesCheck = true;
          }
          if (i - 2 >= 0 && j - 1 >= 0) {
            board[i - 2][j - 1].inWhitesCheck = true;
          }
          if (i - 1 >= 0 && j - 2 >= 0) {
            board[i - 1][j - 2].inWhitesCheck = true;
          }
          if (i + 2 <= 7 && j - 1 >= 0) {
            board[i + 2][j - 1].inWhitesCheck = true;
          }
          if (i - 1 >= 0 && j + 2 <= 7) {
            board[i - 1][j + 2].inWhitesCheck = true;
          }
          if (i - 2 >= 0 && j + 1 <= 7) {
            board[i - 2][j + 1].inWhitesCheck = true;
          }
          if (i + 1 <= 7 && j - 2 >= 0) {
            board[i + 1][j - 2].inWhitesCheck = true;
          }
        } else {
          if (i + 2 <= 7 && j + 1 <= 7) {
            board[i + 2][j + 1].inBlacksCheck = true;
          }
          if (i + 1 <= 7 && j + 2 <= 7) {
            board[i + 1][j + 2].inBlacksCheck = true;
          }
          if (i - 2 >= 0 && j - 1 >= 0) {
            board[i - 2][j - 1].inBlacksCheck = true;
          }
          if (i - 1 >= 0 && j - 2 >= 0) {
            board[i - 1][j - 2].inBlacksCheck = true;
          }
          if (i + 2 <= 7 && j - 1 >= 0) {
            board[i + 2][j - 1].inBlacksCheck = true;
          }
          if (i - 1 >= 0 && j + 2 <= 7) {
            board[i - 1][j + 2].inBlacksCheck = true;
          }
          if (i - 2 >= 0 && j + 1 <= 7) {
            board[i - 2][j + 1].inBlacksCheck = true;
          }
          if (i + 1 <= 7 && j - 2 >= 0) {
            board[i + 1][j - 2].inBlacksCheck = true;
          }
        }
        break;
      case 'p':
        colorMult = board[i][j].isWhite ? 1 : -1;
        assignCheck(i - colorMult, j + 1, board[i][j].isWhite);
        assignCheck(i - colorMult, j - 1, board[i][j].isWhite);
        break;
      case 'k':
        for (int k = -1; k <= 1; k++) {
          for (int l = -1; l <= 1; l++) {
            if (k == 0 && l == 0) {
              continue;
            } else {
              assignCheck(i + k, j + l, board[i][j].isWhite);
            }
          }
        }
        break;
      }
    }
  }
}

// Converts algebraic notation to matrix indexes using the conversion
// functions and then passes the values onto the move checking function
void humanMakeMove(std::string source, std::string target) {
  int fromRank = rankConv(source[1]);
  int fromFile = fileConv(static_cast<char>(std::tolower(source[0])));
  int toRank = rankConv(target[1]);
  int toFile = fileConv(static_cast<char>(std::tolower(target[0])));
  // Without the static cast, due to the std::tolower() function returning the
  // ASCII value the overloaded algebraic notation to array conversion
  // function would do the wrong conversion, returning -58 as the file for 1.
  // e4. Gotta keep these in mind when implementing AI suggestions DEBUGGING
  // td::cout << "humanMakeMove triggered" << std::endl;
  /*std::cout << "Passing these parameters to checkMove function: " << fromRank
            << std::endl
            << fromFile << std::endl
            << toRank << std::endl
            << toFile << std::endl;*/
  checkMove(fromRank, fromFile, toRank, toFile);
}

// Places empty square instances in every space of the board matrix
void fillBoard() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      board[i][j] = Piece();
    }
  }
}

// Hardcoded but does the job, the code is self explanatory.
void setup() {
  board[0][0] = Piece(0, 0, 'r', false, true);
  board[0][1] = Piece(0, 1, 'n', false, true);
  board[0][2] = Piece(0, 2, 'b', false, true);
  board[0][3] = Piece(0, 3, 'q', false, true);
  board[0][4] = Piece(0, 4, 'k', false, true);
  board[0][5] = Piece(0, 5, 'b', false, true);
  board[0][6] = Piece(0, 6, 'n', false, true);
  board[0][7] = Piece(0, 7, 'r', false, true);
  for (int i = 0; i < 8; i++) {
    board[1][i] = Piece(1, i, 'p', false, true);
    board[6][i] = Piece(6, i, 'P', true, true);
  }

  board[7][0] = Piece(7, 0, 'R', true, true);
  board[7][1] = Piece(7, 1, 'N', true, true);
  board[7][2] = Piece(7, 2, 'B', true, true);
  board[7][3] = Piece(7, 3, 'Q', true, true);
  board[7][4] = Piece(7, 4, 'K', true, true);
  board[7][5] = Piece(7, 5, 'B', true, true);
  board[7][6] = Piece(7, 6, 'N', true, true);
  board[7][7] = Piece(7, 7, 'R', true, true);
};

// Iterates over every square in the board and prints its type
void printBoard() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      std::cout << board[i][j].type << ' ';
    }
    // Prints the ranks
    std::cout << rankConv(i) << '\n';
  }
  // Prints the files
  for (int k = 0; k < 8; k++) {
    std::cout << fileConv(k) << ' ';
  }
  std::cout << '\n';
};

int main() {
  fillBoard();
  setup();
  std::cout << "Welcome to chess! No quitting mechanism yet, CTRL+C to quit."
            << '\n';
  while (running) {
    printBoard();
    std::cout << "Where is the piece?" << '\n';
    std::string sourceSquare;
    std::cin >> sourceSquare;
    std::cout << "Where to move the piece?" << '\n';
    std::string targetSquare;
    std::cin >> targetSquare;
    updateCheckMap();
    if (itsSafe(sourceSquare) && itsSafe(targetSquare)) {
      humanMakeMove(sourceSquare, targetSquare);
    }
    if (anyLegalMoves() == false) {
      std::cout << "\033[2J\033[1;1H" << std::flush;
      std::cout << "Game ended" << std::endl;
      printBoard();
      if (board[kingRank(true)][kingFile(true)].inBlacksCheck) {
        std::cout << "Black wins by mate" << std::endl;
        running = false;
      } else if (board[kingRank(false)][kingFile(false)].inWhitesCheck) {
        std::cout << "White wins by mate" << std::endl;
        running = false;
      } else {
        std::cout << "Draw by stalemate" << std::endl;
        running = false;
      }
    }
    if (fiftyMoveCounter >= 100) {
      running = false;
      printBoard();
      std::cout << "Game ended with a draw by the 50 move rule" << std::endl;
    }
    // This piece of gibberish is apparently an escape code for clearing the
    // terminal
    if (running) {
      std::cout << "\033[2J\033[1;1H" << std::flush;
    }
  }
  return 0;
}
