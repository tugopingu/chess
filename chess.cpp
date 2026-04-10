// Licensed with GPL 3.0
#include <SFML/Audio.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

const int WIDTH = 800;
const int SQ_SIDE = 100;
const int PIECE_OFFSET = (100 - 75) / 2;

struct Color {
  int r;
  int g;
  int b;
  Color(int r, int g, int b) {
    this->r = r;
    this->g = g;
    this->b = b;
  }
};

sf::Texture p, r, k, q, b, n;
sf::Texture P, R, K, Q, B, N;

void loadTextures() {
  p.loadFromFile("./assets/png/black/p.png");
  r.loadFromFile("./assets/png/black/r.png");
  k.loadFromFile("./assets/png/black/k.png");
  q.loadFromFile("./assets/png/black/q.png");
  b.loadFromFile("./assets/png/black/b.png");
  n.loadFromFile("./assets/png/black/n.png");

  P.loadFromFile("./assets/png/white/P.png");
  R.loadFromFile("./assets/png/white/R.png");
  K.loadFromFile("./assets/png/white/K.png");
  Q.loadFromFile("./assets/png/white/Q.png");
  B.loadFromFile("./assets/png/white/B.png");
  N.loadFromFile("./assets/png/white/N.png");
}
const Color lightSquare(240, 217, 181);
const Color darkSquare(181, 136, 99);
const Color lightHighlight(205, 210, 106);
const Color darkHighlight(170, 162, 58);
const Color darkSelect(100, 110, 64);
const Color lightSelect(174, 177, 135);

sf::SoundBuffer buffer;
sf::Sound sound;

sf::Font notoSans;

bool whitesMove = true;
int halfMove = 0;
int fiftyMoveCounter = 0;
bool realMove = true;
bool dragging = false;
int lastFromFile;
int lastFromRank;
int lastToRank;
int lastToFile;
bool gameOverSoundAlreadyPlayed;
bool waitingForPromotion = false;
std::vector<std::string> positionHistory;

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

// Returns the position of the king on the board to make the code about mate
// cleaner
int kingRank(bool white) {
  for (int i = 0; i <= 7; i++) {
    for (int j = 0; j <= 7; j++) {
      if (std::tolower(board[i][j].type) == 'k' &&
          board[i][j].isWhite == white) {
        return i;
      }
    }
  }
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
  return 0;
}

void updateCheckMap();

void recordPosition();

// Displays a GUI overlay to let the user select which piece to promote their
// pawn to. Runs every frame, returns a space character if no choice is made.
char promotionBox(sf::RenderWindow &window, bool clicked, bool isWhite,
                  int file) {
  sf::RectangleShape grayOverlay;
  grayOverlay.setSize(sf::Vector2f(800, 800));
  grayOverlay.setPosition(0, 0);
  grayOverlay.setFillColor(sf::Color(0, 0, 0, 128));
  window.draw(grayOverlay);
  sf::Sprite queen, knight, rook, bishop;
  if (isWhite) {
    queen.setTexture(Q);
    knight.setTexture(N);
    rook.setTexture(R);
    bishop.setTexture(B);
    queen.setPosition(sf::Vector2f(file * SQ_SIDE, 0 * SQ_SIDE));
    knight.setPosition(sf::Vector2f(file * SQ_SIDE, 1 * SQ_SIDE));
    rook.setPosition(sf::Vector2f(file * SQ_SIDE, 2 * SQ_SIDE));
    bishop.setPosition(sf::Vector2f(file * SQ_SIDE, 3 * SQ_SIDE));
  } else {
    queen.setTexture(q);
    knight.setTexture(n);
    rook.setTexture(r);
    bishop.setTexture(b);
    queen.setPosition(sf::Vector2f(file * SQ_SIDE, 7 * SQ_SIDE));
    knight.setPosition(sf::Vector2f(file * SQ_SIDE, 6 * SQ_SIDE));
    rook.setPosition(sf::Vector2f(file * SQ_SIDE, 5 * SQ_SIDE));
    bishop.setPosition(sf::Vector2f(file * SQ_SIDE, 4 * SQ_SIDE));
  }
  sf::RectangleShape menu;
  menu.setSize(sf::Vector2f(100, 400));
  menu.setFillColor(sf::Color(200, 200, 200));
  int menuYPos = isWhite ? 0 : 400;
  menu.setPosition(sf::Vector2f(file * SQ_SIDE, menuYPos));
  window.draw(menu);
  sf::RectangleShape highlight;
  highlight.setSize(sf::Vector2f(100, 100));
  highlight.setPosition(sf::Vector2f(
      static_cast<float>((sf::Mouse::getPosition(window).x / SQ_SIDE) *
                         SQ_SIDE),
      static_cast<float>((sf::Mouse::getPosition(window).y / SQ_SIDE) *
                         SQ_SIDE)));
  if (menu.getGlobalBounds().contains(
          window.mapPixelToCoords(sf::Mouse::getPosition(window)))) {
    window.draw(highlight);
  }
  if (queen.getGlobalBounds().contains(
          window.mapPixelToCoords(sf::Mouse::getPosition(window)))) {
    if (clicked) {
      waitingForPromotion = false;
      return 'q';
    }
  }
  if (knight.getGlobalBounds().contains(
          window.mapPixelToCoords(sf::Mouse::getPosition(window)))) {
    if (clicked) {
      waitingForPromotion = false;
      return 'n';
    }
  }
  if (rook.getGlobalBounds().contains(
          window.mapPixelToCoords(sf::Mouse::getPosition(window)))) {
    if (clicked) {
      waitingForPromotion = false;
      return 'r';
    }
  }
  if (bishop.getGlobalBounds().contains(
          window.mapPixelToCoords(sf::Mouse::getPosition(window)))) {
    if (clicked) {
      waitingForPromotion = false;
      return 'b';
    }
  }

  window.draw(queen);
  window.draw(knight);
  window.draw(rook);
  window.draw(bishop);
  return ' ';
}

// Transfers the piece from the source square to the target square to make the
// move. Checks if there is a pawn promotion or an illegal move, and if it is
// the case, rollsback the board to the state before the illegal move
void makeMove(int fromRank, int fromFile, int toRank, int toFile) {
  transaction();
  bool isWhiteMove = board[fromRank][fromFile].isWhite;
  bool isCapture = board[toRank][toFile].type != '.';
  bool isPawnMove = std::tolower(board[fromRank][fromFile].type) == 'p';
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
      (toRank == 0 || toRank == 7) && realMove) {
    waitingForPromotion = true;
  }
  // Increment counter
  whitesMove = !whitesMove;
  halfMove++;
  if (isPawnMove || isCapture) {
    fiftyMoveCounter = 0;
  } else {
    fiftyMoveCounter++;
  }
  recordPosition();
  if (realMove) {
    lastFromRank = fromRank;
    lastFromFile = fromFile;
    lastToRank = toRank;
    lastToFile = toFile;
    if (isCapture) {
      buffer.loadFromFile("./assets/ogg/Capture.ogg");
    } else {
      buffer.loadFromFile("./assets/ogg/Move.ogg");
    }
    sound.setBuffer(buffer);
    sound.play();
  }
  // std::cout << "makeMove triggered" << std::endl; // DEBUG
}

// Checks if the move is valid. If it is, it passes the values onto the makeMove
// function
void checkMove(int fromRank, int fromFile, int toRank, int toFile) {
  int currentHalfMove = halfMove;
  if (waitingForPromotion) {
    return;
  }
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
  // King logic
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
      if (realMove) {
        lastFromRank = fromRank;
        lastFromFile = fromFile;
        lastToRank = toRank;
        lastToFile = toFile;
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
      if (realMove) {
        lastFromRank = fromRank;
        lastFromFile = fromFile;
        lastToRank = toRank;
        lastToFile = toFile;
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
        if (realMove) {
          buffer.loadFromFile("./assets/ogg/Capture.ogg");
          sound.setBuffer(buffer);
          sound.play();
        }
      }
    }
    break;
  }
}

// Returns a boolean value if it is a draw caused by insufficient material.
// Checks if there are any queens, rook or pawns. If none, it checks if both
// players have less than 2 knights or bishops. If it is the case, it returns
// true for draw.
bool insufficientMaterial() {
  int blackKnightOrBishopCount = 0;
  int whiteKnightOrBishopCount = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (std::tolower(board[i][j].type) == 'q' ||
          std::tolower(board[i][j].type) == 'r' ||
          std::tolower(board[i][j].type) == 'p') {
        return false;
      } else if (board[i][j].type == 'n' || board[i][j].type == 'b') {
        blackKnightOrBishopCount++;
      } else if (board[i][j].type == 'N' || board[i][j].type == 'B') {
        whiteKnightOrBishopCount++;
      }
    }
  }
  if (whiteKnightOrBishopCount >= 2 || blackKnightOrBishopCount >= 2) {
    return false;
  } else {
    return true;
  }
}

// Uses the position history to check if there are three matching elements,
// which is a threefold repetition
bool threefoldRepetition() {
  for (int i = 0; i < positionHistory.size(); i++) {
    int positionOccurences = 0;
    for (int j = i; j < positionHistory.size(); j++) {
      if (positionHistory.at(i) == positionHistory.at(j)) {
        positionOccurences++;
      }
    }
    if (positionOccurences >= 3) {
      return true;
    }
  }
  return false;
}

// Adds the current state of the board to the position history string vector,
// later to be used for checking draw by threefold repetition
void recordPosition() {
  std::string currentPosition = "";
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      currentPosition.push_back(board[i][j].type);
    }
  }
  positionHistory.push_back(currentPosition);
}

// Brute forces moves to find if there are no legal moves, which is checkmate or
// stalemate. It operates on the real board and rolls back the board to the
// previous state to not change the state of the board
bool anyLegalMoves() {
  if (waitingForPromotion) {
    return true;
  }
  Piece tempBoard[8][8];
  realMove = false;
  std::vector<std::string> currentPositionHistory = positionHistory;
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
          positionHistory = currentPositionHistory;
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

// Used by the updateCheckMap function to cleanly assign checks to the squares
// of the board
void assignCheck(int rank, int file, bool white) {
  if (rank <= 7 && rank >= 0 && file <= 7 && file >= 0) {
    if (white) {
      board[rank][file].inWhitesCheck = true;
    } else {
      board[rank][file].inBlacksCheck = true;
    }
  }
}

// Updates the check state of the board, copied mostly from the checkMove
// function
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
        // Should have implemented the assignCheck function earlier, what is
        // this?
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
  recordPosition();
};

// Displays the squares on the board before the pieces are drawn
void displayBackground(sf::RenderWindow &window) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      sf::RectangleShape square;
      square.setSize(sf::Vector2f(SQ_SIDE, SQ_SIDE));
      square.setPosition(i * SQ_SIDE, j * SQ_SIDE);
      if ((i + j) % 2 == 0) {
        square.setFillColor(
            sf::Color(lightSquare.r, lightSquare.g, lightSquare.b));
      } else {
        square.setFillColor(
            sf::Color(darkSquare.r, darkSquare.g, darkSquare.b));
      }
      window.draw(square);
    }
  }
}

// Draws the pieces on the window
void displayBoard(sf::RenderWindow &window) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      sf::Sprite piece;
      char type = board[i][j].type;
      switch (type) {
      case 'p':
        piece.setTexture(p);
        break;
      case 'P':
        piece.setTexture(P);
        break;
      case 'r':
        piece.setTexture(r);
        break;
      case 'R':
        piece.setTexture(R);
        break;
      case 'q':
        piece.setTexture(q);
        break;
      case 'Q':
        piece.setTexture(Q);
        break;
      case 'n':
        piece.setTexture(n);
        break;
      case 'N':
        piece.setTexture(N);
        break;
      case 'b':
        piece.setTexture(b);
        break;
      case 'B':
        piece.setTexture(B);
        break;
      case 'k':
        piece.setTexture(k);
        break;
      case 'K':
        piece.setTexture(K);
        break;
      }
      piece.setPosition(j * SQ_SIDE, i * SQ_SIDE);
      window.draw(piece);
    }
  }
}

// Draws the pieces, but it is used for when a piece is dragged. It displays the
// dragged piece on the mouse instead of its source square
void displayBoardDragging(sf::RenderWindow &window, int draggedPieceRank,
                          int draggedPieceFile) {
  sf::RectangleShape square;
  int hoverFile, hoverRank;
  hoverRank = sf::Mouse::getPosition(window).y / SQ_SIDE;
  hoverFile = sf::Mouse::getPosition(window).x / SQ_SIDE;
  square.setSize(sf::Vector2f(SQ_SIDE, SQ_SIDE));
  square.setPosition(hoverFile * SQ_SIDE, hoverRank * SQ_SIDE);
  if ((hoverFile + hoverRank) % 2 == 0) {
    square.setFillColor(sf::Color(lightSelect.r, lightSelect.g, lightSelect.b));
  } else {
    square.setFillColor(sf::Color(darkSelect.r, darkSelect.g, darkSelect.b));
  }
  window.draw(square);

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      sf::Sprite piece;
      char type = board[i][j].type;
      switch (type) {
      case 'p':
        piece.setTexture(p);
        break;
      case 'P':
        piece.setTexture(P);
        break;
      case 'r':
        piece.setTexture(r);
        break;
      case 'R':
        piece.setTexture(R);
        break;
      case 'q':
        piece.setTexture(q);
        break;
      case 'Q':
        piece.setTexture(Q);
        break;
      case 'n':
        piece.setTexture(n);
        break;
      case 'N':
        piece.setTexture(N);
        break;
      case 'b':
        piece.setTexture(b);
        break;
      case 'B':
        piece.setTexture(B);
        break;
      case 'k':
        piece.setTexture(k);
        break;
      case 'K':
        piece.setTexture(K);
        break;
      }
      piece.setPosition(j * SQ_SIDE, i * SQ_SIDE);
      if (draggedPieceFile == j && draggedPieceRank == i) {
        continue;
      }
      window.draw(piece);
    }
  }
  sf::Sprite piece;
  char type = board[draggedPieceRank][draggedPieceFile].type;
  switch (type) {
  case 'p':
    piece.setTexture(p);
    break;
  case 'P':
    piece.setTexture(P);
    break;
  case 'r':
    piece.setTexture(r);
    break;
  case 'R':
    piece.setTexture(R);
    break;
  case 'q':
    piece.setTexture(q);
    break;
  case 'Q':
    piece.setTexture(Q);
    break;
  case 'n':
    piece.setTexture(n);
    break;
  case 'N':
    piece.setTexture(N);
    break;
  case 'b':
    piece.setTexture(b);
    break;
  case 'B':
    piece.setTexture(B);
    break;
  case 'k':
    piece.setTexture(k);
    break;
  case 'K':
    piece.setTexture(K);
    break;
  }
  piece.setPosition(sf::Mouse::getPosition(window).x - 50,
                    sf::Mouse::getPosition(window).y - 50);
  window.draw(piece);
}

// Displays a dialog box, asking the user to quit or start a new game
std::string gameEndBox(sf::RenderWindow &window, std::string text,
                       bool clicked) {
  sf::RectangleShape grayOverlay;
  grayOverlay.setSize(sf::Vector2f(800, 800));
  grayOverlay.setPosition(0, 0);
  grayOverlay.setFillColor(sf::Color(0, 0, 0, 128));
  window.draw(grayOverlay);
  sf::RectangleShape messageBox;
  messageBox.setSize(sf::Vector2f(650, 300));
  messageBox.setPosition(sf::Vector2f(75, 250));
  messageBox.setFillColor(sf::Color(50, 50, 50));
  window.draw(messageBox);

  sf::Text title;
  title.setFont(notoSans);
  title.setString("Game Ended");
  title.setCharacterSize(36);
  title.setFillColor(sf::Color(255, 255, 255));
  sf::FloatRect titleBounds = title.getGlobalBounds();
  title.setPosition(sf::Vector2f((800 - titleBounds.width) / (double)2, 270));
  window.draw(title);

  sf::Text message;
  message.setFont(notoSans);
  message.setString(text);
  message.setCharacterSize(36);
  message.setFillColor(sf::Color(255, 255, 255));
  sf::FloatRect messageBounds = message.getGlobalBounds();
  message.setPosition(
      sf::Vector2f((800 - messageBounds.width) / (double)2, 330));
  window.draw(message);

  sf::RectangleShape newGameButton;
  newGameButton.setSize(sf::Vector2f(150, 50));
  newGameButton.setPosition(sf::Vector2f(200, 420));
  newGameButton.setFillColor(sf::Color(50, 50, 50));
  newGameButton.setOutlineColor(sf::Color(64, 64, 64));
  newGameButton.setOutlineThickness(2);
  sf::FloatRect newGameButtonBounds = newGameButton.getLocalBounds();
  window.draw(newGameButton);

  sf::RectangleShape quitButton;
  quitButton.setSize(sf::Vector2f(150, 50));
  quitButton.setPosition(sf::Vector2f(450, 420));
  quitButton.setFillColor(sf::Color(50, 50, 50));
  quitButton.setOutlineColor(sf::Color(64, 64, 64));
  quitButton.setOutlineThickness(2);
  sf::FloatRect quitButtonBounds = quitButton.getLocalBounds();
  window.draw(quitButton);

  sf::Text newGameText;
  newGameText.setFont(notoSans);
  newGameText.setString("New Game");
  newGameText.setCharacterSize(25);
  newGameText.setFillColor(sf::Color(255, 255, 255));
  sf::FloatRect newGameBounds = newGameText.getLocalBounds();
  newGameText.setOrigin(newGameBounds.width / (double)2,
                        newGameBounds.height / (double)2);
  newGameText.setPosition(sf::Vector2f(
      newGameButton.getPosition().x + newGameButtonBounds.width / (double)2 - 2,
      newGameButton.getPosition().y + 20));
  window.draw(newGameText);

  sf::Text quitText;
  quitText.setFont(notoSans);
  quitText.setString("Quit");
  quitText.setCharacterSize(30);
  quitText.setFillColor(sf::Color(255, 255, 255));
  sf::FloatRect quitBounds = quitText.getLocalBounds();
  quitText.setOrigin(quitBounds.width / (double)2,
                     quitBounds.height / (double)2);
  quitText.setPosition(sf::Vector2f(quitButton.getPosition().x +
                                        quitButtonBounds.width / (double)2 - 5,
                                    quitButton.getPosition().y + 20));
  window.draw(quitText);
  if (quitButton.getGlobalBounds().contains(
          window.mapPixelToCoords(sf::Mouse::getPosition(window)))) {
    quitButton.setFillColor(sf::Color(64, 64, 64));
    if (clicked) {
      return "quit";
    }
  } else {
    quitButton.setFillColor(sf::Color(50, 50, 50));
  }
  window.draw(quitButton);
  window.draw(quitText);

  if (newGameButton.getGlobalBounds().contains(
          window.mapPixelToCoords(sf::Mouse::getPosition(window)))) {
    newGameButton.setFillColor(sf::Color(64, 64, 64));
    if (clicked) {
      return "newGame";
    }
  } else {
    newGameButton.setFillColor(sf::Color(50, 50, 50));
  }
  window.draw(newGameButton);
  window.draw(newGameText);

  if (!gameOverSoundAlreadyPlayed) {
    gameOverSoundAlreadyPlayed = true;
    buffer.loadFromFile("./assets/ogg/GenericNotify.ogg");
    sound.setBuffer(buffer);
    sound.play();
  }
  return "null";
}

int main() {
  notoSans.loadFromFile("./assets/NotoSans-VariableFont_wdth,wght.ttf");
  loadTextures();
  bool firstClickMade = false;
  sf::Vector2i sourceClickPos;
  sf::Vector2i targetClickPos;
  sf::RenderWindow window(sf::VideoMode(800, 800), "Chess",
                          sf::Style::Titlebar | sf::Style::Close);
  window.setFramerateLimit(120);
  sf::Image icon;
  icon.loadFromFile("./assets/icon.png");
  window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
  while (window.isOpen()) {
    waitingForPromotion = false;
    positionHistory.clear();
    fillBoard();
    setup();
    whitesMove = true;
    halfMove = 0;
    fiftyMoveCounter = 0;
    std::string gameEndBoxOutput = "";
    gameOverSoundAlreadyPlayed = false;
    while (true) {
      realMove = true;
      int fromRank, fromFile, toRank, toFile;
      displayBackground(window);
      sf::RectangleShape checkHighlight;
      checkHighlight.setSize(sf::Vector2f(SQ_SIDE, SQ_SIDE));
      checkHighlight.setFillColor(sf::Color(210, 70, 50));
      if (board[kingRank(true)][kingFile(true)].inBlacksCheck) {
        checkHighlight.setPosition(
            sf::Vector2f(kingFile(true) * SQ_SIDE, kingRank(true) * SQ_SIDE));
        window.draw(checkHighlight);
      }
      if (board[kingRank(false)][kingFile(false)].inWhitesCheck) {
        checkHighlight.setPosition(
            sf::Vector2f(kingFile(false) * SQ_SIDE, kingRank(false) * SQ_SIDE));
        window.draw(checkHighlight);
      }

      if (halfMove > 0) {
        sf::RectangleShape source;
        source.setSize(sf::Vector2f(SQ_SIDE, SQ_SIDE));
        source.setPosition(
            sf::Vector2f(lastFromFile * SQ_SIDE, lastFromRank * SQ_SIDE));
        if ((lastFromFile + lastFromRank) % 2 == 0) {
          source.setFillColor(
              sf::Color(lightHighlight.r, lightHighlight.g, lightHighlight.b));
        } else {
          source.setFillColor(
              sf::Color(darkHighlight.r, darkHighlight.g, darkHighlight.b));
        }
        window.draw(source);
        sf::RectangleShape target;
        target.setSize(sf::Vector2f(SQ_SIDE, SQ_SIDE));
        target.setPosition(
            sf::Vector2f(lastToFile * SQ_SIDE, lastToRank * SQ_SIDE));
        if ((lastToFile + lastToRank) % 2 == 0) {
          target.setFillColor(
              sf::Color(lightHighlight.r, lightHighlight.g, lightHighlight.b));
        } else {
          target.setFillColor(
              sf::Color(darkHighlight.r, darkHighlight.g, darkHighlight.b));
        }
        window.draw(target);
      }
      if (firstClickMade && board[fromRank][fromFile].type != '.' &&
          board[fromRank][fromFile].isWhite == whitesMove) {
        // click highlighting
        sf::RectangleShape square;
        square.setSize(sf::Vector2f(SQ_SIDE, SQ_SIDE));
        square.setPosition(fromFile * SQ_SIDE, fromRank * SQ_SIDE);
        if ((fromFile + fromRank) % 2 == 0) {
          square.setFillColor(
              sf::Color(lightSelect.r, lightSelect.g, lightSelect.b));
        } else {
          square.setFillColor(
              sf::Color(darkSelect.r, darkSelect.g, darkSelect.b));
        }
        window.draw(square);
      }
      if (dragging) {
        displayBoardDragging(window, fromRank, fromFile);
      } else {
        displayBoard(window);
      }
      sf::Event event;
      bool clicked = false;
      while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
          window.close();
        }
        if (event.type == sf::Event::MouseButtonPressed) {
          if (event.mouseButton.button == sf::Mouse::Left) {
            clicked = true;
            if (!firstClickMade) {
              sourceClickPos = sf::Mouse::getPosition(window);
              firstClickMade = !firstClickMade;
              fromRank = sourceClickPos.y / SQ_SIDE;
              fromFile = sourceClickPos.x / SQ_SIDE;
            } else {
              targetClickPos = sf::Mouse::getPosition(window);
              toRank = targetClickPos.y / SQ_SIDE;
              toFile = targetClickPos.x / SQ_SIDE;
              int currentHalfMove = halfMove;
              checkMove(fromRank, fromFile, toRank, toFile);
              if (currentHalfMove == halfMove) {
                sourceClickPos = sf::Mouse::getPosition(window);
                fromRank = sourceClickPos.y / SQ_SIDE;
                fromFile = sourceClickPos.x / SQ_SIDE;
              } else {
                firstClickMade = !firstClickMade;
              }
            }
          }
        }
        if (event.type == sf::Event::MouseButtonReleased &&
            event.mouseButton.button == sf::Mouse::Left && firstClickMade &&
            (std::abs(sourceClickPos.x - sf::Mouse::getPosition(window).x) >
                 10 ||
             std::abs(sourceClickPos.y - sf::Mouse::getPosition(window).y) >
                 10)) {
          targetClickPos = sf::Mouse::getPosition(window);
          toRank = targetClickPos.y / SQ_SIDE;
          toFile = targetClickPos.x / SQ_SIDE;
          checkMove(fromRank, fromFile, toRank, toFile);
          firstClickMade = !firstClickMade;
        }
      }
      if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) &&
          firstClickMade &&
          (std::abs(sourceClickPos.x - sf::Mouse::getPosition(window).x) > 10 ||
           std::abs(sourceClickPos.y - sf::Mouse::getPosition(window).y) >
               10)) {
        dragging = true;
      } else {
        dragging = false;
      }
      if (waitingForPromotion) {
        bool whitePromotion = board[lastToRank][lastToFile].isWhite;
        char promotionChoice =
            promotionBox(window, clicked, whitePromotion, lastToFile);
        if (promotionChoice != ' ') {
          board[lastToRank][lastToFile].type =
              board[lastToRank][lastToFile].isWhite
                  ? std::toupper(promotionChoice)
                  : std::tolower(promotionChoice);
          waitingForPromotion = false;
        }
      }
      if (threefoldRepetition()) {
        gameEndBoxOutput = gameEndBox(
            window, "Draw by threefold position repetition", clicked);
      }
      if (fiftyMoveCounter >= 100) {
        gameEndBoxOutput =
            gameEndBox(window, "Draw by fifty move rule", clicked);
      }

      if (anyLegalMoves() == false) {
        if (board[kingRank(true)][kingFile(true)].inBlacksCheck) {
          gameEndBoxOutput = gameEndBox(window, "Black wins by mate", clicked);
        } else if (board[kingRank(false)][kingFile(false)].inWhitesCheck) {
          gameEndBoxOutput = gameEndBox(window, "White wins by mate", clicked);
        } else {
          gameEndBoxOutput = gameEndBox(window, "Draw by stalemate", clicked);
        }
      }
      if (insufficientMaterial()) {
        gameEndBoxOutput =
            gameEndBox(window, "Draw by insufficient mating material", clicked);
      }
      window.display();
      if (gameEndBoxOutput == "quit") {
        window.close();
        return 0;
      }
      if (gameEndBoxOutput == "newGame") {
        break;
      }
    }
  }
  return 0;
}
