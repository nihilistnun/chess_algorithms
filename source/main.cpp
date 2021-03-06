#include "includes.h"

#include "user_interface.h"
#include "algorithms.h"

#include "debug.h"


//---------------------------------------------------------------------------------------
// Global variable
//---------------------------------------------------------------------------------------
Game* current_game = NULL;

//---------------------------------------------------------------------------------------
// Helper
// Auxiliar functions to determine if a move is valid, etc
//---------------------------------------------------------------------------------------
bool isMoveValid(Chess::Position present, Chess::Position future, Chess::EnPassant* S_enPassant, Chess::Castling* S_castling, Chess::Promotion* S_promotion)
{
	bool bValid = false;

	char chPiece = current_game->getPieceAtPosition(present.iRow, present.iColumn);

	// ----------------------------------------------------
	// 1. Is the piece  allowed to move in that direction?
	// ----------------------------------------------------
	switch (toupper(chPiece))
	{
	case 'P':
	{
		// Wants to move forward
		if (future.iColumn == present.iColumn)
		{
			// Simple move forward
			if ((Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 1) ||
				(Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 1))
			{
				if (EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow, future.iColumn))
				{
					bValid = true;
				}
			}

			// Double move forward
			else if ((Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 2) ||
				(Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 2))
			{
				// This is only allowed if the pawn is in its original place
				if (Chess::isWhitePiece(chPiece))
				{
					if (EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow - 1, future.iColumn) &&
						EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow, future.iColumn) &&
						1 == present.iRow)
					{
						bValid = true;
					}
				}
				else // if ( isBlackPiece(chPiece) )
				{
					if (EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow + 1, future.iColumn) &&
						EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow, future.iColumn) &&
						6 == present.iRow)
					{
						bValid = true;
					}
				}
			}
			else
			{
				// This is invalid
				return false;
			}
		}

		// The "en passant" move
		else if ((Chess::isWhitePiece(chPiece) && 4 == present.iRow && 5 == future.iRow && 1 == abs(future.iColumn - present.iColumn)) ||
			(Chess::isBlackPiece(chPiece) && 3 == present.iRow && 2 == future.iRow && 1 == abs(future.iColumn - present.iColumn)))
		{
			if (current_game->rounds.size() < 1) {
				//necessary for custom endgames where there is no previous round
				return false;
			}
			// It is only valid if last move of the opponent was a double move forward by a pawn on a adjacent column
			string last_move = current_game->getLastMove();

			// Parse the line
			Chess::Position LastMoveFrom;
			Chess::Position LastMoveTo;
			current_game->parseMove(last_move, &LastMoveFrom, &LastMoveTo);

			// First of all, was it a pawn?
			char chLstMvPiece = current_game->getPieceAtPosition(LastMoveTo.iRow, LastMoveTo.iColumn);

			if (toupper(chLstMvPiece) != 'P')
			{
				return false;
			}

			// Did the pawn have a double move forward and was it an adjacent column?
			if (2 == abs(LastMoveTo.iRow - LastMoveFrom.iRow) && 1 == abs(LastMoveFrom.iColumn - present.iColumn))
			{
				//cout << "En passant move!\n";
				bValid = true;

				S_enPassant->bApplied = true;
				S_enPassant->PawnCaptured.iRow = LastMoveTo.iRow;
				S_enPassant->PawnCaptured.iColumn = LastMoveTo.iColumn;
			}
		}

		// Wants to capture a piece
		else if (1 == abs(future.iColumn - present.iColumn))
		{
			if ((Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 1) || (Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 1))
			{
				// Only allowed if there is something to be captured in the square
				if (EMPTY_SQUARE != current_game->getPieceAtPosition(future.iRow, future.iColumn))
				{
					bValid = true;
					cout << "Pawn captured a piece!\n";
				}
			}
		}
		else
		{
			// This is invalid
			return false;
		}

		// If a pawn reaches its eight rank, it must be promoted to another piece
		if ((Chess::isWhitePiece(chPiece) && 7 == future.iRow) ||
			(Chess::isBlackPiece(chPiece) && 0 == future.iRow))
		{
			//cout << "Pawn must be promoted!\n";
			S_promotion->bApplied = true;
		}
	}
	break;

	case 'R':
	{
		// Horizontal move
		if ((future.iRow == present.iRow) && (future.iColumn != present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFree(present, future, Chess::HORIZONTAL))
			{
				bValid = true;
			}
		}
		// Vertical move
		else if ((future.iRow != present.iRow) && (future.iColumn == present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFree(present, future, Chess::VERTICAL))
			{
				bValid = true;
			}
		}
	}
	break;

	case 'N':
	{
		if ((2 == abs(future.iRow - present.iRow)) && (1 == abs(future.iColumn - present.iColumn)))
		{
			bValid = true;
		}

		else if ((1 == abs(future.iRow - present.iRow)) && (2 == abs(future.iColumn - present.iColumn)))
		{
			bValid = true;
		}
	}
	break;

	case 'B':
	{
		// Diagonal move
		if (abs(future.iRow - present.iRow) == abs(future.iColumn - present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFree(present, future, Chess::DIAGONAL))
			{
				bValid = true;
			}
		}
	}
	break;

	case 'Q':
	{
		// Horizontal move
		if ((future.iRow == present.iRow) && (future.iColumn != present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFree(present, future, Chess::HORIZONTAL))
			{
				bValid = true;
			}
		}
		// Vertical move
		else if ((future.iRow != present.iRow) && (future.iColumn == present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFree(present, future, Chess::VERTICAL))
			{
				bValid = true;
			}
		}

		// Diagonal move
		else if (abs(future.iRow - present.iRow) == abs(future.iColumn - present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFree(present, future, Chess::DIAGONAL))
			{
				bValid = true;
			}
		}
	}
	break;

	case 'K':
	{
		// Horizontal move by 1
		if ((future.iRow == present.iRow) && (1 == abs(future.iColumn - present.iColumn)))
		{
			bValid = true;
		}

		// Vertical move by 1
		else if ((future.iColumn == present.iColumn) && (1 == abs(future.iRow - present.iRow)))
		{
			bValid = true;
		}

		// Diagonal move by 1
		else if ((1 == abs(future.iRow - present.iRow)) && (1 == abs(future.iColumn - present.iColumn)))
		{
			bValid = true;
		}

		// Castling
		else if ((future.iRow == present.iRow) && (2 == abs(future.iColumn - present.iColumn)))
		{
			// Castling is only allowed in these circunstances:

			// 1. King is not in check
			if (true == current_game->playerKingInCheck())
			{
				return false;
			}

			// 2. No pieces in between the king and the rook
			if (false == current_game->isPathFree(present, future, Chess::HORIZONTAL))
			{
				return false;
			}

			// 3. King and rook must not have moved yet;
			// 4. King must not pass through a square that is attacked by an enemy piece
			if (future.iColumn > present.iColumn)
			{
				// if future.iColumn is greather, it means king side
				if (false == current_game->castlingAllowed(Chess::Side::KING_SIDE, Chess::getPieceColor(chPiece)))
				{
					createNextMessage("Castling to the king side is not allowed.\n");
					return false;
				}
				else
				{
					// Check if the square that the king skips is not under attack
					Chess::UnderAttack square_skipped = current_game->isUnderAttack(present.iRow, present.iColumn + 1, current_game->getCurrentTurn());
					if (false == square_skipped.bUnderAttack)
					{
						// Fill the S_castling structure
						S_castling->bApplied = true;

						// Present position of the rook
						S_castling->rook_before.iRow = present.iRow;
						S_castling->rook_before.iColumn = present.iColumn + 3;

						// Future position of the rook
						S_castling->rook_after.iRow = future.iRow;
						S_castling->rook_after.iColumn = present.iColumn + 1; // future.iColumn -1

						bValid = true;
					}
				}
			}
			else //if (future.iColumn < present.iColumn)
			{
				// if present.iColumn is greather, it means queen side
				if (false == current_game->castlingAllowed(Chess::Side::QUEEN_SIDE, Chess::getPieceColor(chPiece)))
				{
					createNextMessage("Castling to the queen side is not allowed.\n");
					return false;
				}
				else
				{
					// Check if the square that the king skips is not attacked
					Chess::UnderAttack square_skipped = current_game->isUnderAttack(present.iRow, present.iColumn - 1, current_game->getCurrentTurn());
					if (false == square_skipped.bUnderAttack)
					{
						// Fill the S_castling structure
						S_castling->bApplied = true;

						// Present position of the rook
						S_castling->rook_before.iRow = present.iRow;
						S_castling->rook_before.iColumn = present.iColumn - 4;

						// Future position of the rook
						S_castling->rook_after.iRow = future.iRow;
						S_castling->rook_after.iColumn = present.iColumn - 1; // future.iColumn +1

						bValid = true;
					}
				}
			}
		}
	}
	break;

	default:
	{
		cout << "!!!!Should not reach here. Invalid piece: " << char(chPiece) << "\n\n\n";
	}
	break;
	}

	// If it is a move in an invalid direction, do not even bother to check the rest
	if (false == bValid)
	{
		cout << "Piece is not allowed to move to that square\n";
		return false;
	}


	// -------------------------------------------------------------------------
	// 2. Is there another piece of the same color on the destination square?
	// -------------------------------------------------------------------------
	if (current_game->isSquareOccupied(future.iRow, future.iColumn))
	{
		char chAuxPiece = current_game->getPieceAtPosition(future.iRow, future.iColumn);
		if (Chess::getPieceColor(chPiece) == Chess::getPieceColor(chAuxPiece))
		{
			cout << "Position is already taken by a piece of the same color\n";
			return false;
		}
	}

	// ----------------------------------------------
	// 3. Would the king be in check after the move?
	// ----------------------------------------------
	if (true == current_game->wouldKingBeInCheck(chPiece, present, future, S_enPassant))
	{
		cout << "Move would put player's king in check\n";
		return false;
	}

	return bValid;
}

void makeTheMove(Chess::Position present, Chess::Position future, Chess::EnPassant* S_enPassant, Chess::Castling* S_castling, Chess::Promotion* S_promotion)
{
	char chPiece = current_game->getPieceAtPosition(present.iRow, present.iColumn);

	// -----------------------
	// Captured a piece?
	// -----------------------
	if (current_game->isSquareOccupied(future.iRow, future.iColumn))
	{
		char chAuxPiece = current_game->getPieceAtPosition(future.iRow, future.iColumn);

		if (Chess::getPieceColor(chPiece) != Chess::getPieceColor(chAuxPiece))
		{
			createNextMessage(Chess::describePiece(chAuxPiece) + " captured!\n");
		}
		else
		{
			cout << "Error. We should not be making this move\n";
			throw("Error. We should not be making this move");
		}
	}
	else if (true == S_enPassant->bApplied)
	{
		//createNextMessage("Pawn captured by \"en passant\" move!\n");
	}

	if ((true == S_castling->bApplied))
	{
		//createNextMessage("Castling applied!\n");
	}

	current_game->movePiece(present, future, S_enPassant, S_castling, S_promotion);
}

//---------------------------------------------------------------------------------------
// Commands
// Functions to handle the commands of the program
// New game, Move, Undo, Save game, Load game, etc
//---------------------------------------------------------------------------------------
void newGame(void)
{
	if (NULL != current_game)
	{
		delete current_game;
	}

	current_game = new Game();
}

void undoMove(void)
{
	if (false == current_game->undoIsPossible())
	{
		createNextMessage("Undo is not possible now!\n");
		return;
	}

	current_game->undoLastMove();
	createNextMessage("Last move was undone\n");
}

void movePiece(void)
{
	std::string to_record;

	// Get user input for the piece they want to move
	cout << "Choose piece to be moved. (example: A1 or b2): ";

	std::string move_from;
	getline(cin, move_from);

	if (move_from.length() > 2)
	{
		createNextMessage("You should type only two characters (column and row)\n");
		return;
	}

	Chess::Position present;
	present.iColumn = move_from[0];
	present.iRow = move_from[1];

	// ---------------------------------------------------
	// Did the user pick a valid piece?
	// Must check if:
	// - It's inside the board (A1-H8)
	// - There is a piece in the square
	// - The piece is consistent with the player's turn
	// ---------------------------------------------------
	present.iColumn = toupper(present.iColumn);

	if (present.iColumn < 'A' || present.iColumn > 'H')
	{
		createNextMessage("Invalid column.\n");
		return;
	}

	if (present.iRow < '0' || present.iRow > '8')
	{
		createNextMessage("Invalid row.\n");
		return;
	}

	// Put in the string to be logged
	to_record += present.iColumn;
	to_record += present.iRow;
	to_record += "-";

	// Convert column from ['A'-'H'] to [0x00-0x07]
	present.iColumn = present.iColumn - 'A';

	// Convert row from ['1'-'8'] to [0x00-0x07]
	present.iRow = present.iRow - '1';

	char chPiece = current_game->getPieceAtPosition(present.iRow, present.iColumn);
	cout << "Piece is " << char(chPiece) << "\n";

	if (0x20 == chPiece)
	{
		createNextMessage("You picked an EMPTY square.\n");
		return;
	}

	if (Chess::WHITE_PIECE == current_game->getCurrentTurn())
	{
		if (false == Chess::isWhitePiece(chPiece))
		{
			createNextMessage("It is WHITE's turn and you picked a BLACK piece\n");
			return;
		}
	}
	else
	{
		if (false == Chess::isBlackPiece(chPiece))
		{
			createNextMessage("It is BLACK's turn and you picked a WHITE piece\n");
			return;
		}
	}

	// ---------------------------------------------------
	// Get user input for the square to move to
	// ---------------------------------------------------
	cout << "Move to: ";
	std::string move_to;
	getline(cin, move_to);

	if (move_to.length() > 2)
	{
		createNextMessage("You should type only two characters (column and row)\n");
		return;
	}

	// ---------------------------------------------------
	// Did the user pick a valid house to move?
	// Must check if:
	// - It's inside the board (A1-H8)
	// - The move is valid
	// ---------------------------------------------------
	Chess::Position future;
	future.iColumn = move_to[0];
	future.iRow = move_to[1];

	future.iColumn = toupper(future.iColumn);

	if (future.iColumn < 'A' || future.iColumn > 'H')
	{
		createNextMessage("Invalid column.\n");
		return;
	}

	if (future.iRow < '0' || future.iRow > '8')
	{
		createNextMessage("Invalid row.\n");
		return;
	}

	// Put in the string to be logged
	to_record += future.iColumn;
	to_record += future.iRow;

	// Convert columns from ['A'-'H'] to [0x00-0x07]
	future.iColumn = future.iColumn - 'A';

	// Convert row from ['1'-'8'] to [0x00-0x07]
	future.iRow = future.iRow - '1';

	// Check if it is not the exact same square
	if (future.iRow == present.iRow && future.iColumn == present.iColumn)
	{
		createNextMessage("[Invalid] You picked the same square!\n");
		return;
	}

	// Is that move allowed?
	Chess::EnPassant  S_enPassant = { 0 };
	Chess::Castling   S_castling = { 0 };
	Chess::Promotion  S_promotion = { 0 };

	if (false == isMoveValid(present, future, &S_enPassant, &S_castling, &S_promotion))
	{
		createNextMessage("[Invalid] Piece can not move to that square!\n");
		return;
	}

	// ---------------------------------------------------
	// Promotion: user most choose a piece to
	// replace the pawn
	// ---------------------------------------------------
	if (S_promotion.bApplied == true)
	{
		cout << "Promote to (Q, R, N, B): ";
		std::string piece;
		getline(cin, piece);

		if (piece.length() > 1)
		{
			createNextMessage("You should type only one character (Q, R, N or B)\n");
			return;
		}

		char chPromoted = toupper(piece[0]);

		if (chPromoted != 'Q' && chPromoted != 'R' && chPromoted != 'N' && chPromoted != 'B')
		{
			createNextMessage("Invalid character.\n");
			return;
		}

		S_promotion.chBefore = current_game->getPieceAtPosition(present.iRow, present.iColumn);

		if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
		{
			S_promotion.chAfter = toupper(chPromoted);
		}
		else
		{
			S_promotion.chAfter = tolower(chPromoted);
		}

		to_record += '=';
		to_record += toupper(chPromoted); // always log with a capital letter
	}

	// ---------------------------------------------------
	// Log the move: do it prior to making the move
	// because we need the getCurrentTurn()
	// ---------------------------------------------------
	current_game->logMove(to_record);

	// ---------------------------------------------------
	// Make the move
	// ---------------------------------------------------
	makeTheMove(present, future, &S_enPassant, &S_castling, &S_promotion);

	// ---------------------------------------------------------------
	// Check if this move we just did put the oponent's king in check
	// Keep in mind that player turn has already changed
	// ---------------------------------------------------------------
	if (true == current_game->playerKingInCheck())
	{
		if (true == current_game->isCheckMate())
		{
			if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
			{
				appendToNextMessage("Checkmate! Black wins the game!\n");
			}
			else
			{
				appendToNextMessage("Checkmate! White wins the game!\n");
			}
		}
		else
		{
			// Add to the string with '+=' because it's possible that
			// there is already one message (e.g., piece captured)
			if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
			{
				appendToNextMessage("White king is in check!\n");
			}
			else
			{
				appendToNextMessage("Black king is in check!\n");
			}
		}
	}

	return;
}

void saveGame(void)
{
	string file_name;
	cout << "Type file name to be saved (no extension): ";

	getline(cin, file_name);
	file_name += ".dat";

	std::ofstream ofs(file_name);
	if (ofs.is_open())
	{
		// Write the date and time of save operation
		auto time_now = std::chrono::system_clock::now();
		std::time_t end_time = std::chrono::system_clock::to_time_t(time_now);
		ofs << "[Chess console] Saved at: " << std::ctime(&end_time);

		// Write the moves
		for (unsigned i = 0; i < current_game->rounds.size(); i++)
		{
			ofs << current_game->rounds[i].white_move.c_str() << " | " << current_game->rounds[i].black_move.c_str() << "\n";
		}

		ofs.close();
		createNextMessage("Game saved as " + file_name + "\n");
	}
	else
	{
		cout << "Error creating file! Save failed\n";
	}

	return;
}

void loadGame(void)
{
	string file_name;
	cout << "Type file name to be loaded (no extension): ";

	getline(cin, file_name);
	file_name += ".dat";

	std::ifstream ifs(file_name);

	if (ifs)
	{
		// First, reset the pieces
		if (NULL != current_game)
		{
			delete current_game;
		}

		current_game = new Game();

		// Now, read the lines from the file and then make the moves
		std::string line;

		while (std::getline(ifs, line))
		{
			// Skip lines that starts with "[]"
			if (0 == line.compare(0, 1, "["))
			{
				continue;
			}

			// There might be one or two moves in the line
			string loaded_move[2];

			// Find the separator and subtract one
			std::size_t separator = line.find(" |");

			// For the first move, read from the beginning of the string until the separator
			loaded_move[0] = line.substr(0, separator);

			// For the second move, read from the separator until the end of the string (omit second parameter)
			loaded_move[1] = line.substr(line.find("|") + 2);

			for (int i = 0; i < 2 && loaded_move[i] != ""; i++)
			{
				// Parse the line
				Chess::Position from;
				Chess::Position to;

				char chPromoted = 0;

				current_game->parseMove(loaded_move[i], &from, &to, &chPromoted);

				// Check if line is valid
				if (from.iColumn < 0 || from.iColumn > 7 ||
					from.iRow < 0 || from.iRow    > 7 ||
					to.iColumn < 0 || to.iColumn   > 7 ||
					to.iRow < 0 || to.iRow      > 7)
				{
					createNextMessage("[Invalid] Can't load this game because there are invalid lines!\n");

					// Clear everything and return
					current_game = new Game();
					return;
				}

				// Is that move allowed? (should be because we already validated before saving)
				Chess::EnPassant S_enPassant = { 0 };
				Chess::Castling  S_castling = { 0 };
				Chess::Promotion S_promotion = { 0 };

				if (false == isMoveValid(from, to, &S_enPassant, &S_castling, &S_promotion))
				{
					createNextMessage("[Invalid] Can't load this game because there are invalid moves!\n");

					// Clear everything and return
					current_game = new Game();
					return;
				}

				// ---------------------------------------------------
				// A promotion occurred
				// ---------------------------------------------------
				if (S_promotion.bApplied == true)
				{
					if (chPromoted != 'Q' && chPromoted != 'R' && chPromoted != 'N' && chPromoted != 'B')
					{
						createNextMessage("[Invalid] Can't load this game because there is an invalid promotion!\n");

						// Clear everything and return
						current_game = new Game();
						return;
					}

					S_promotion.chBefore = current_game->getPieceAtPosition(from.iRow, from.iColumn);

					if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
					{
						S_promotion.chAfter = toupper(chPromoted);
					}
					else
					{
						S_promotion.chAfter = tolower(chPromoted);
					}
				}


				// Log the move
				current_game->logMove(loaded_move[i]);

				// Make the move
				makeTheMove(from, to, &S_enPassant, &S_castling, &S_promotion);
			}
		}

		// Extra line after the user input
		createNextMessage("Game loaded from " + file_name + "\n");

		return;
	}
	else
	{
		createNextMessage("Error loading " + file_name + ". Creating a new game instead\n");
		current_game = new Game();
		return;
	}
}

//---------------------------------------------------------------------------------------
// New methods for Algorithm play
//---------------------------------------------------------------------------------------

void newEndGame(int setup)
{
	if (NULL != current_game)
	{
		delete current_game;
	}

	current_game = new Game(setup);
}

//no print is move valid checker
bool isMoveValidNP(Chess::Position present, Chess::Position future, Chess::EnPassant* S_enPassant, Chess::Castling* S_castling, Chess::Promotion* S_promotion)
{
	bool bValid = false;

	char chPiece = current_game->getPieceAtPosition(present.iRow, present.iColumn);

	// ----------------------------------------------------
	// 1. Is the piece  allowed to move in that direction?
	// ----------------------------------------------------
	switch (toupper(chPiece))
	{
	case 'P':
	{
		// Wants to move forward
		if (future.iColumn == present.iColumn)
		{
			// Simple move forward
			if ((Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 1) ||
				(Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 1))
			{
				if (EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow, future.iColumn))
				{
					bValid = true;
				}
			}

			// Double move forward
			else if ((Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 2) ||
				(Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 2))
			{
				// This is only allowed if the pawn is in its original place
				if (Chess::isWhitePiece(chPiece))
				{
					if (EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow - 1, future.iColumn) &&
						EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow, future.iColumn) &&
						1 == present.iRow)
					{
						bValid = true;
					}
				}
				else // if ( isBlackPiece(chPiece) )
				{
					if (EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow + 1, future.iColumn) &&
						EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow, future.iColumn) &&
						6 == present.iRow)
					{
						bValid = true;
					}
				}
			}
			else
			{
				// This is invalid
				return false;
			}
		}

		// The "en passant" move
		else if ((Chess::isWhitePiece(chPiece) && 4 == present.iRow && 5 == future.iRow && 1 == abs(future.iColumn - present.iColumn)) ||
			(Chess::isBlackPiece(chPiece) && 3 == present.iRow && 2 == future.iRow && 1 == abs(future.iColumn - present.iColumn)))
		{
			// It is only valid if last move of the opponent was a double move forward by a pawn on a adjacent column
			if (current_game->rounds.size() < 1) {
				//necessary for custom endgames where there is no previous round
				return false;
			}
			string last_move = current_game->getLastMove();

			// Parse the line
			Chess::Position LastMoveFrom;
			Chess::Position LastMoveTo;
			current_game->parseMove(last_move, &LastMoveFrom, &LastMoveTo);

			// First of all, was it a pawn?
			char chLstMvPiece = current_game->getPieceAtPosition(LastMoveTo.iRow, LastMoveTo.iColumn);

			if (toupper(chLstMvPiece) != 'P')
			{
				return false;
			}

			// Did the pawn have a double move forward and was it an adjacent column?
			if (2 == abs(LastMoveTo.iRow - LastMoveFrom.iRow) && 1 == abs(LastMoveFrom.iColumn - present.iColumn))
			{
				bValid = true;

				S_enPassant->bApplied = true;
				S_enPassant->PawnCaptured.iRow = LastMoveTo.iRow;
				S_enPassant->PawnCaptured.iColumn = LastMoveTo.iColumn;
			}
		}

		// Wants to capture a piece
		else if (1 == abs(future.iColumn - present.iColumn))
		{
			if ((Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 1) || (Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 1))
			{
				// Only allowed if there is something to be captured in the square
				if (EMPTY_SQUARE != current_game->getPieceAtPosition(future.iRow, future.iColumn))
				{
					bValid = true;
				}
			}
		}
		else
		{
			// This is invalid
			return false;
		}

		// If a pawn reaches its eight rank, it must be promoted to another piece
		if ((Chess::isWhitePiece(chPiece) && 7 == future.iRow) ||
			(Chess::isBlackPiece(chPiece) && 0 == future.iRow))
		{
			S_promotion->bApplied = true;
		}
	}
	break;

	case 'R':
	{
		// Horizontal move
		if ((future.iRow == present.iRow) && (future.iColumn != present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFreeNP(present, future, Chess::HORIZONTAL))
			{
				bValid = true;
			}
		}
		// Vertical move
		else if ((future.iRow != present.iRow) && (future.iColumn == present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFreeNP(present, future, Chess::VERTICAL))
			{
				bValid = true;
			}
		}
	}
	break;

	case 'N':
	{
		if ((2 == abs(future.iRow - present.iRow)) && (1 == abs(future.iColumn - present.iColumn)))
		{
			bValid = true;
		}

		else if ((1 == abs(future.iRow - present.iRow)) && (2 == abs(future.iColumn - present.iColumn)))
		{
			bValid = true;
		}
	}
	break;

	case 'B':
	{
		// Diagonal move
		if (abs(future.iRow - present.iRow) == abs(future.iColumn - present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFreeNP(present, future, Chess::DIAGONAL))
			{
				bValid = true;
			}
		}
	}
	break;

	case 'Q':
	{
		// Horizontal move
		if ((future.iRow == present.iRow) && (future.iColumn != present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFreeNP(present, future, Chess::HORIZONTAL))
			{
				bValid = true;
			}
		}
		// Vertical move
		else if ((future.iRow != present.iRow) && (future.iColumn == present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFreeNP(present, future, Chess::VERTICAL))
			{
				bValid = true;
			}
		}

		// Diagonal move
		else if (abs(future.iRow - present.iRow) == abs(future.iColumn - present.iColumn))
		{
			// Check if there are no pieces on the way
			if (current_game->isPathFreeNP(present, future, Chess::DIAGONAL))
			{
				bValid = true;
			}
		}
	}
	break;

	case 'K':
	{
		// Horizontal move by 1
		if ((future.iRow == present.iRow) && (1 == abs(future.iColumn - present.iColumn)))
		{
			bValid = true;
		}

		// Vertical move by 1
		else if ((future.iColumn == present.iColumn) && (1 == abs(future.iRow - present.iRow)))
		{
			bValid = true;
		}

		// Diagonal move by 1
		else if ((1 == abs(future.iRow - present.iRow)) && (1 == abs(future.iColumn - present.iColumn)))
		{
			bValid = true;
		}

		// Castling is disabled
		//else if ((future.iRow == present.iRow) && (2 == abs(future.iColumn - present.iColumn)))
		//{
		//	// Castling is only allowed in these circunstances:

		//	// 1. King is not in check
		//	if (true == current_game->playerKingInCheck())
		//	{
		//		return false;
		//	}

		//	// 2. No pieces in between the king and the rook
		//	if (false == current_game->isPathFreeNP(present, future, Chess::HORIZONTAL))
		//	{
		//		return false;
		//	}

		//	// 3. King and rook must not have moved yet;
		//	// 4. King must not pass through a square that is attacked by an enemy piece
		//	if (future.iColumn > present.iColumn)
		//	{
		//		// if future.iColumn is greather, it means king side
		//		if (false == current_game->castlingAllowed(Chess::Side::KING_SIDE, Chess::getPieceColor(chPiece)))
		//		{
		//			return false;
		//		}
		//		else
		//		{
		//			// Check if the square that the king skips is not under attack
		//			Chess::UnderAttack square_skipped = current_game->isUnderAttack(present.iRow, present.iColumn + 1, current_game->getCurrentTurn());
		//			if (false == square_skipped.bUnderAttack)
		//			{
		//				// Fill the S_castling structure
		//				S_castling->bApplied = true;

		//				// Present position of the rook
		//				S_castling->rook_before.iRow = present.iRow;
		//				S_castling->rook_before.iColumn = present.iColumn + 3;

		//				// Future position of the rook
		//				S_castling->rook_after.iRow = future.iRow;
		//				S_castling->rook_after.iColumn = present.iColumn + 1; // future.iColumn -1

		//				bValid = true;
		//			}
		//		}
		//	}
		//	else //if (future.iColumn < present.iColumn)
		//	{
		//		// if present.iColumn is greather, it means queen side
		//		if (false == current_game->castlingAllowed(Chess::Side::QUEEN_SIDE, Chess::getPieceColor(chPiece)))
		//		{
		//			return false;
		//		}
		//		else
		//		{
		//			// Check if the square that the king skips is not attacked
		//			Chess::UnderAttack square_skipped = current_game->isUnderAttack(present.iRow, present.iColumn - 1, current_game->getCurrentTurn());
		//			if (false == square_skipped.bUnderAttack)
		//			{
		//				// Fill the S_castling structure
		//				S_castling->bApplied = true;

		//				// Present position of the rook
		//				S_castling->rook_before.iRow = present.iRow;
		//				S_castling->rook_before.iColumn = present.iColumn - 4;

		//				// Future position of the rook
		//				S_castling->rook_after.iRow = future.iRow;
		//				S_castling->rook_after.iColumn = present.iColumn - 1; // future.iColumn +1

		//				bValid = true;
		//			}
		//		}
		//	}
		//}
	}
	break;

	default:
	{
		cout << "!!!!Should not reach here. Invalid piece: " << char(chPiece) << "\n\n\n";
	}
	break;
	}

	// If it is a move in an invalid direction, do not even bother to check the rest
	if (false == bValid)
	{
		return false;
	}


	// -------------------------------------------------------------------------
	// 2. Is there another piece of the same color on the destination square?
	// -------------------------------------------------------------------------
	if (current_game->isSquareOccupied(future.iRow, future.iColumn))
	{
		char chAuxPiece = current_game->getPieceAtPosition(future.iRow, future.iColumn);
		if (Chess::getPieceColor(chPiece) == Chess::getPieceColor(chAuxPiece))
		{
			return false;
		}
	}

	// ----------------------------------------------
	// 3. Would the king be in check after the move?
	// ----------------------------------------------
	if (true == current_game->wouldKingBeInCheck(chPiece, present, future, S_enPassant))
	{
		return false;
	}
	return bValid;
}

//auto promotes to queen
bool movePiece(Algorithms::Move move)
{
	std::string to_record;
	// ---------------------------------------------------
	// Did the user pick a valid piece?
	// Must check if:
	// - It's inside the board (A1-H8)
	// - There is a piece in the square
	// - The piece is consistent with the player's turn
	// ---------------------------------------------------
	move.present.iColumn = toupper(move.present.iColumn + 'A');
	move.present.iRow++;

	if (move.present.iColumn < 'A' || move.present.iColumn > 'H')
	{
		return false;
	}

	if (move.present.iRow < 0 || move.present.iRow > 8)
	{
		return false;
	}

	// Put in the string to be logged
	to_record += move.present.iColumn;
	to_record += move.present.iRow + '0';
	to_record += "-";

	// Convert column from ['A'-'H'] to [0x00-0x07]
	move.present.iColumn = move.present.iColumn - 'A';

	// Convert row from ['1'-'8'] to [0x00-0x07]
	move.present.iRow = move.present.iRow--;

	char chPiece = current_game->getPieceAtPosition(move.present.iRow, move.present.iColumn);
	//cout << "Piece is " << char(chPiece) << "\n";

	if (0x20 == chPiece)
	{
		return false;
	}

	if (Chess::WHITE_PIECE == current_game->getCurrentTurn())
	{
		if (false == Chess::isWhitePiece(chPiece))
		{
			return false;
		}
	}
	else
	{
		if (false == Chess::isBlackPiece(chPiece))
		{
			return false;
		}
	}
	// ---------------------------------------------------
	// Did the user pick a valid house to move?
	// Must check if:
	// - It's inside the board (A1-H8)
	// - The move is valid
	// ---------------------------------------------------
	move.future.iColumn = toupper(move.future.iColumn + 'A');
	move.future.iRow++;

	if (move.future.iColumn < 'A' || move.future.iColumn > 'H')
	{
		return false;
	}

	if (move.future.iRow < 0 || move.future.iRow > 8)
	{
		return false;
	}

	// Put in the string to be logged
	to_record += move.future.iColumn;
	to_record += move.future.iRow + '0';

	// Convert columns from ['A'-'H'] to [0x00-0x07]
	move.future.iColumn = move.future.iColumn - 'A';

	// Convert row from ['1'-'8'] to [0x00-0x07]
	move.future.iRow = move.future.iRow--;

	// Check if it is not the exact same square
	if (move.future.iRow == move.present.iRow && move.future.iColumn == move.present.iColumn)
	{
		return false;
	}

	//redundant check, can be REMOVED
	if (false == isMoveValidNP(move.present, move.future, &move.S_enPassant, &move.S_castling, &move.S_promotion))
	{
		return false;
	}

	// ---------------------------------------------------
	// Promotion: user most choose a piece to
	// replace the pawn
	// Promotion is defaulted to queen for algorithms
	// ---------------------------------------------------
	if (move.S_promotion.bApplied == true)
	{
		char chPromoted = 'Q';

		move.S_promotion.chBefore = current_game->getPieceAtPosition(move.present.iRow, move.present.iColumn);

		if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
		{
			move.S_promotion.chAfter = toupper(chPromoted);
		}
		else
		{
			move.S_promotion.chAfter = tolower(chPromoted);
		}

		to_record += '=';
		to_record += toupper(chPromoted); // always log with a capital letter
	}

	// ---------------------------------------------------
	// Log the move: do it prior to making the move
	// because we need the getCurrentTurn()
	// ---------------------------------------------------
	current_game->logMove(to_record);

	// ---------------------------------------------------
	// Make the move
	// ---------------------------------------------------
	makeTheMove(move.present, move.future, &move.S_enPassant, &move.S_castling, &move.S_promotion);

	// ---------------------------------------------------------------
	// Check if this move we just did put the oponent's king in check
	// Keep in mind that player turn has already changed
	// ---------------------------------------------------------------
	if (true == current_game->playerKingInCheck())
	{
		if (true == current_game->isCheckMate())
		{
			if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
			{
				appendToNextMessage("Checkmate! Black wins the game!\n");
			}
			else
			{
				appendToNextMessage("Checkmate! White wins the game!\n");
			}
		}
		else
		{
			// Add to the string with '+=' because it's possible that
			// there is already one message (e.g., piece captured)
			if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
			{
				//appendToNextMessage("White king is in check!\n");
			}
			else
			{
				//appendToNextMessage("Black king is in check!\n");
			}
		}
	}

	/*printLogo();
	printSituation(*current_game);
	printBoard(*current_game);*/
	return true;
}

//all valid moves for player
vector<Algorithms::Move> allValidMoves(Chess::Player player) {
	vector<Algorithms::Move> validMoves;
	if (current_game->isFinished())
		return validMoves;
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			Chess::Position present{ i, j };
			char chPiece = current_game->getPieceAtPosition(present);
			if (chPiece != 0x20 && Chess::getPieceColor(chPiece) == player) {
				//for every piece of the player
				for (int k = 0; k < 8; k++) {
					for (int l = 0; l < 8; l++) {
						Chess::Position future{ k, l };
						char chTarget = current_game->getPieceAtPosition(future);
						if (chTarget == 0x20 || Chess::getPieceColor(chTarget) != player) {
							//every destination that is empty or an enemy piece
							Chess::EnPassant  S_enPassant = { 0 };
							Chess::Castling   S_castling = { 0 };
							Chess::Promotion  S_promotion = { 0 };
							if (true == isMoveValidNP(present, future, &S_enPassant, &S_castling, &S_promotion)) {
								//log a valid move
								//if (toupper(chTarget) == 'K') {//should not occur
								//	printLogo();
								//	printSituation(*current_game);
								//	printBoard(*current_game);
								//	bool debug = isMoveValidNP(present, future, &S_enPassant, &S_castling, &S_promotion);
								//}
								validMoves.push_back(Algorithms::Move{ present, future, S_enPassant, S_castling, S_promotion });
							}
						}
					}
				}
				//end of second double for loop
			}
		}
	}
	return validMoves;
}

//---------------------------------------------------------------------------------------
// Methods for results logging
//---------------------------------------------------------------------------------------
//template <typename T>
//void record(int endgameNumber, bool minimax, bool white, int param, string fileName, vector<T>& data) {
//	string columnName = fileName;
//	string path = "../results/";
//	if (minimax) {
//		path += "minimax/";
//		fileName = (white ? "_MM_W_" : "_MM_") + std::to_string(param) + "_" + fileName;
//	}
//	else {
//		path += "mcts/";
//		fileName = (white ? "_MCTS_W_" : "_MCTS_") + std::to_string(param) + "_" + fileName;
//	}
//	path += "EG" + std::to_string(endgameNumber);
//	path += fileName + ".csv";
//
//	std::ifstream check(path);
//	std::ofstream file;
//	file.open(path, std::ofstream::out | std::ofstream::app);
//	//write data with file <<
//	if (!check)
//		file << "Endgame, White, Parameter, Move," << columnName << "\n";
//	check.close();
//	for (int i = 0; i < data.size(); i++) {
//		file << endgameNumber << ',' << (white ? "True" : "False") << ',' <<
//			param << ',' << (i + 1) << "," << data[i] << "\n";
//	}
//	file.close();
//}
template <typename T, typename U, typename P>
void record(int endgameNumber, bool minimax, bool white, int param, vector<T>& evals, vector<U>& num, vector<P>& time) {
	string path = "../results/";
	path += (minimax ? "Minimax.csv" : "MCTS.csv");
	std::ifstream check(path);
	std::ofstream file;
	file.open(path, std::ofstream::out | std::ofstream::app);
	//write data with file <<
	if (!check)
		file << "Endgame,White,Parameter,Move,Evaluation_Function," << (minimax ? "Evaluation_Number" : "Nodes_Number") << ",Time\n";
	check.close();
	for (int i = 0; i < evals.size(); i++) {
		file << endgameNumber << ',' << (white ? "True" : "False") << ',' <<
			param << ',' << (i + 1) << "," << evals[i] << "," << num[i] << "," << time[i] << "\n";
	}
	file.close();
}

void minimaxHistory(int endgameNum, int param, bool white, string opponent, int oppoParam,
	int win, int draw, int lose, int moveNum, int totalEvals, float totalTime, int totalEvals2, float totalTime2) {
	string path = "../results/minimaxHistory.csv";
	std::ifstream check(path);
	std::ofstream file;
	file.open(path, std::ofstream::out | std::ofstream::app);
	if (!check)
		file << "Endgame,Algo,Parameter,White,Opponenet,Opponent_Parameter,Win,Draw,Lose,Move_Num,Total_Evaluations,Total_Time,Oppo_Total_Num,Oppo_Total_Time\n";
	check.close();
	//write data
	file << endgameNum << ',' << "Minimax" << ',' << param << ',' << (white ? "True" : "False") << ',' <<
		opponent << ',' << oppoParam << ',' << win << ',' << draw << ',' << lose << ',' << moveNum << ',' <<
		totalEvals << ',' << totalTime << ',' << totalEvals2 << ',' << totalTime2 << "\n";
	file.close();
}

void mctsHistory(int endgameNum, int param, bool white, string opponent, int oppoParam,
	int win, int draw, int lose, int moveNum, int totalNodes, float totalTime, int totalNodes2, float totalTime2) {
	string path = "../results/mctsHistory.csv";
	std::ifstream check(path);
	std::ofstream file;
	file.open(path, std::ofstream::out | std::ofstream::app);
	if (!check)
		file << "Endgame,Algo,Parameter,White,Opponenet,Opponent_Parameter,Win,Draw,Lose,Move_Num,Total_Nodes,Total_Time,Oppo_Total_Num,Oppo_Total_Time\n";
	check.close();
	//write data
	file << endgameNum << ',' << "MCTS" << ',' << param << ',' << (white ? "True" : "False") << ',' <<
		opponent << ',' << oppoParam << ',' << win << ',' << draw << ',' << lose << ',' << moveNum << ',' <<
		totalNodes << ',' << totalTime << ',' << totalNodes2 << ',' << totalTime2 << "\n";
	file.close();
}

int main()
{
	bool bRun = true;
	Algorithms* algo = nullptr;
	int endgameNo = -1;

	// Clear screen an print the logo
	clearScreen();
	printLogo();

	string input = "";

	while (bRun)
	{
		printMessage();
		printMenu();

		// Get input from user
		cout << "Type here: ";
		getline(cin, input);

		if (input.length() != 1)
		{
			cout << "Invalid option. Type one letter only\n\n";
			continue;
		}

		try
		{
			switch (input[0])
			{
			case 'N':
			case 'n':
			{
				do {
					cout << "Endgame number: ";
					getline(cin, input);
				} while (!(!input.empty() && std::find_if(input.begin(), input.end(), [](unsigned char c) {
					return !std::isdigit(c); }) == input.end()));
				endgameNo = stoi(input);
				newEndGame(stoi(input));
				delete algo;
				algo = new Algorithms(current_game, allValidMoves, movePiece);
				clearScreen();
				printLogo();
				printSituation(*current_game);
				printBoard(*current_game);
			}
			break;

			case 'M':
			case 'm':
			{
				if (NULL != current_game)
				{
					if (current_game->isFinished())
					{
						cout << "This game has already finished!\n";
					}
					else
					{
						movePiece();
						//clearScreen();
						printLogo();
						printSituation(*current_game);
						printBoard(*current_game);
					}
				}
				else
				{
					cout << "No game running!\n";
				}

			}
			break;

			case 'E':
			case 'e':
			{
				if (NULL != current_game)
				{
					if (current_game->isFinished())
					{
						cout << "This game has already finished!\n";
					}
					else
					{
						//clearScreen();
						printLogo();
						printSituation(*current_game);
						printBoard(*current_game);
						cout << "Evalue: " << current_game->evaluate() << "\n";
						//vector<Algorithms::Move> moves = allValidMoves(current_game->getCurrentTurn() == 0 ? Chess::WHITE_PLAYER : Chess::BLACK_PLAYER);
						//for (const auto& i : moves)
						//	cout << '(' << char('A' + i.present.iColumn) << i.present.iRow+1
						//	<< '-' << char('A' + i.future.iColumn) << i.future.iRow+1 << ')';
					}
				}
				else
				{
					cout << "No game running!\n";
				}

			}
			break;

			case 'R'://minimax as white
			case 'r':
			{

				int value, depth = 5, time = 3;
				for (int x = 0; x >= 0; x++) {//each endgame
					endgameNo = x % 5;
					cout << x << ':' << endgameNo << "\n";
					newEndGame(endgameNo);
					delete algo;
					algo = new Algorithms(current_game, allValidMoves, movePiece);

					//logging vars
					//minimax
					vector<int> mmEval;
					vector<int> mmNum;
					vector<float> mmTime;
					float totalTime = 0;
					//int totalEvals = 0;
					//mcts
					vector<int> mctsEval;
					vector<int> mctsNodes;
					vector<float> mctsTime;
					float totalTime2 = 0;
					//int totalNodes2 = 0;
					//algo vars
					bool player1 = true;
					while (!current_game->isFinished()) {
						if (totalTime > 600 || totalTime2 > 600)//caps games to 10 minutes
							current_game->setStaleMate();
						if (player1)
							value = algo->minimaxSearchTimed(current_game->getCurrentTurn() == Chess::WHITE_PLAYER);
						else
							algo->monteCarloTreeSearchTimed(time);
						algo->doBestMove();
						printLogo();
						printSituation(*current_game);
						printBoard(*current_game);
						if (player1 && algo != nullptr) {
							mmEval.push_back(value);
							mmNum.push_back(algo->gamesEvalauted);
							mmTime.push_back(algo->minimaxTimeElapsed);
							totalTime += algo->minimaxTimeElapsed;
							//totalEvals += algo->gamesEvalauted;
						}
						else {
							mctsEval.push_back(current_game->evaluate());
							mctsNodes.push_back(algo->nodesCreated);
							mctsTime.push_back(algo->mctsActualTime);
							totalTime2 += algo->mctsActualTime;
							//totalNodes += algo->nodesCreated;
						}
						player1 = !player1;
					}
					//write data
					record(endgameNo, true, true, depth, mmEval, mmNum, mmTime);
					record(endgameNo, false, false, time, mctsEval, mctsNodes, mctsTime);
					int win = 0, draw = 0, lose = 0;
					if (current_game->winner != 0)
						if (current_game->winner > 0)
							win = 1;
						else
							lose = 1;
					else
						draw = 1;
					minimaxHistory(endgameNo, depth, true, "MCTS", time, win, draw, lose, mmEval.size(), algo->totalEvaluated, totalTime, algo->totalNodesCreated, totalTime2);
					mctsHistory(endgameNo, time, false, "Minimax", depth, lose, draw, win, mmEval.size(), algo->totalNodesCreated, totalTime2, algo->totalEvaluated, totalTime);

				}

			}
			break;

			case 'T'://mcts as whitte
			case 't':
			{

				int value, depth = 5, time = 3;
				for (int x = 0; x >= 0; x++) {//each endgame
					endgameNo = x % 5;
					cout << x << ':' << endgameNo << "\n";
					newEndGame(endgameNo);
					delete algo;
					algo = new Algorithms(current_game, allValidMoves, movePiece);

					//logging vars
					//minimax
					vector<int> mmEval;
					vector<int> mmNum;
					vector<float> mmTime;
					float totalTime = 0;
					//int totalEvals = 0;
					//mcts
					vector<int> mctsEval;
					vector<int> mctsNodes;
					vector<float> mctsTime;
					float totalTime2 = 0;
					//int totalNodes2 = 0;
					//algo vars
					bool player1 = false;
					while (!current_game->isFinished()) {
						if (totalTime > 600 || totalTime2 > 600)//caps games to 10 minutes
							current_game->setStaleMate();
						if (player1)
							value = algo->minimaxSearchTimed(current_game->getCurrentTurn() == Chess::WHITE_PLAYER);
						else
							algo->monteCarloTreeSearchTimed(time);
						algo->doBestMove();
						/*printLogo();
						printSituation(*current_game);
						printBoard(*current_game);*/
						if (player1 && algo != nullptr) {
							mmEval.push_back(value);
							mmNum.push_back(algo->gamesEvalauted);
							mmTime.push_back(algo->minimaxTimeElapsed);
							totalTime += algo->minimaxTimeElapsed;
							//totalEvals += algo->gamesEvalauted;
						}
						else {
							mctsEval.push_back(current_game->evaluate());
							mctsNodes.push_back(algo->nodesCreated);
							mctsTime.push_back(algo->mctsActualTime);
							totalTime2 += algo->mctsActualTime;
							//totalNodes += algo->nodesCreated;
						}
						player1 = !player1;
					}
					//write data
					record(endgameNo, true, false, depth, mmEval, mmNum, mmTime);
					record(endgameNo, false, true, time, mctsEval, mctsNodes, mctsTime);
					int win = 0, draw = 0, lose = 0;
					if (current_game->winner != 0)
						if (current_game->winner > 0)
							win = 1;
						else
							lose = 1;
					else
						draw = 1;
					minimaxHistory(endgameNo, depth, false, "MCTS", time, lose, draw, win, mctsEval.size(), algo->totalEvaluated, totalTime, algo->totalNodesCreated, totalTime2);
					mctsHistory(endgameNo, time, true, "Minimax", depth, win, draw, lose, mctsEval.size(), algo->totalNodesCreated, totalTime2, algo->totalEvaluated, totalTime);

				}
			}
			break;


			case 'Y'://minimax vs minimax
			case 'y':
			{

				int value, depth1 = 2, depth2 = 1;
				for (int x = 0; x < 5; x++) {//each endgame
					for (int i = 1; i < 6; i++) {//depth 1 params
						depth1 = i;
						for (int j = 1; j < 6; j++) {//depth2 params
							if (i == j)
								continue;
							for (int k = 0; k < 5; k++) {//repeat each game 5 times
								cout << x << ":" << i << ":" << j << ":" << k << "\n";
								depth2 = j;
								endgameNo = x;
								newEndGame(endgameNo);
								delete algo;
								algo = new Algorithms(current_game, allValidMoves, movePiece);

								//logging vars
								//minimax
								vector<int> mmEval;
								vector<int> mmNum;
								vector<float> mmTime;
								float totalTime = 0;
								int totalEvals = 0;
								//minimax 2
								vector<int> mmEval2;
								vector<int> mmNum2;
								vector<float> mmTime2;
								float totalTime2 = 0;
								int totalEvals2 = 0;
								//algo vars
								bool player1 = true;

								//code here
								player1 = true;
								while (!current_game->isFinished()) {
									if (totalTime > 300 || totalTime2 > 300)//caps games to 10 minutes
										current_game->setStaleMate();
									if (player1)
										algo->setMaxDepth(depth1);
									else
										algo->setMaxDepth(depth2);
									value = algo->minimaxSearchTimed(current_game->getCurrentTurn() == Chess::WHITE_PLAYER);

									algo->doBestMove();
									/*printLogo();
									printSituation(*current_game);
									printBoard(*current_game);*/
									if (player1 && algo != nullptr) {
										mmEval.push_back(value);
										mmNum.push_back(algo->gamesEvalauted);
										mmTime.push_back(algo->minimaxTimeElapsed);
										totalTime += algo->minimaxTimeElapsed;
										totalEvals += algo->gamesEvalauted;
									}
									else {
										mmEval2.push_back(value);
										mmNum2.push_back(algo->gamesEvalauted);
										mmTime2.push_back(algo->minimaxTimeElapsed);
										totalTime2 += algo->minimaxTimeElapsed;
										totalEvals2 += algo->gamesEvalauted;
									}
									player1 = !player1;
								}
								//write data
								record(endgameNo, true, true, depth1, mmEval, mmNum, mmTime);
								record(endgameNo, true, false, depth2, mmEval2, mmNum2, mmTime2);
								int win = 0, draw = 0, lose = 0;
								if (current_game->winner != 0)
									if (current_game->winner > 0)
										win = 1;
									else
										lose = 1;
								else
									draw = 1;
								minimaxHistory(endgameNo, depth1, true, "Minimax", depth2, win, draw, lose, mmEval.size(), totalEvals, totalTime, totalEvals2, totalTime2);


							}
						}
					}
				}

			}
			break;

			case 'U'://mcts vs mcts
			case 'u':
			{
				//start

				int time1 = 1;//seconds
				int time2 = 1;//seconds
				for (int x = 4; x >= 0; x++) {//each endgame
					endgameNo = x % 5;
					for (int i = 3; i < 6; i++) {//time 1 params
						time1 = i;
						for (int j = 2; j < 6; j++) {//time 2 params
							if (i == j)
								continue;
							cout << x << ":" << i << ":" << j << "\n";
							time2 = j;
							newEndGame(endgameNo);
							delete algo;
							algo = new Algorithms(current_game, allValidMoves, movePiece);

							//logging vars
							//mcts 1
							vector<int> mctsEval;
							vector<int> mctsNodes;
							vector<float> mctsTime;
							float totalTime = 0;
							int totalNodes = 0;
							//mcts 2
							vector<int> mctsEval2;
							vector<int> mctsNodes2;
							vector<float> mctsTime2;
							float totalTime2 = 0;
							int totalNodes2 = 0;
							//algo vars
							bool player1 = true;

							//code here
							player1 = true;
							while (!current_game->isFinished()) {
								if (player1)
									algo->monteCarloTreeSearchTimed(time1);
								else
									algo->monteCarloTreeSearchTimed(time2);
								algo->doBestMove();
								/*printLogo();
								printSituation(*current_game);
								printBoard(*current_game);*/
								if (player1 && algo != nullptr) {
									mctsEval.push_back(current_game->evaluate());
									mctsNodes.push_back(algo->nodesCreated);
									mctsTime.push_back(algo->mctsActualTime);
									totalTime += algo->mctsActualTime;
									totalNodes += algo->nodesCreated;
								}
								else {
									mctsEval2.push_back(current_game->evaluate());
									mctsNodes2.push_back(algo->nodesCreated);
									mctsTime2.push_back(algo->mctsActualTime);
									totalTime2 += algo->mctsActualTime;
									totalNodes2 += algo->nodesCreated;
								}
								player1 = !player1;

								if (totalTime > 300 || totalTime2 > 300) {//caps games to 5 minutes
									current_game->setStaleMate();
									current_game->winner = 0;
								}
							}
							//write data
							record(endgameNo, false, true, time1, mctsEval, mctsNodes, mctsTime);
							record(endgameNo, false, false, time2, mctsEval2, mctsNodes2, mctsTime2);
							int win = 0, draw = 0, lose = 0;
							if (current_game->winner != 0)
								if (current_game->winner > 0)
									win = 1;
								else
									lose = 1;
							else
								draw = 1;
							mctsHistory(endgameNo, time1, true, "MCTS", time2, win, draw, lose, mctsEval.size(), totalNodes, totalTime, totalNodes2, totalTime2);


						}
					}
				}

				//end
			}
			break;

			case 'I'://just mcts once
			case 'i':
			{
				if (NULL != current_game)
				{
					if (current_game->isFinished())
					{
						cout << "This game has already finished!\n";
					}
					else
					{
						//clearScreen();
						algo->monteCarloTreeSearchTimed(1);
						algo->doBestMove();
						printLogo();
						printSituation(*current_game);
						printBoard(*current_game);
						cout << "Evalue: " << algo->mctsEval << "\n";
						cout << "Actual time: " << algo->mctsActualTime << "\n";
					}
				}
				else
				{
					cout << "No game running!\n";
				}

			}
			break;

			case 'O'://just minimax once
			case 'o':
			{
				if (NULL != current_game)
				{
					if (current_game->isFinished())
					{
						cout << "This game has already finished!\n";
					}
					else
					{
						//clearScreen();
						int value;
						value = algo->minimaxSearchTimed(current_game->getCurrentTurn() == Chess::WHITE_PLAYER);
						algo->doBestMove();
						printLogo();
						printSituation(*current_game);
						printBoard(*current_game);
						//do something
						cout << "Minimax Value: " << value << "\n";
					}
				}
				else
				{
					cout << "No game running!\n";
				}

			}
			break;

			case 'Q':
			case 'q':
			{
				bRun = false;
			}
			break;

			default:
			{
				cout << "Option does not exist\n\n";
			}
			break;

			}

		}
		catch (const char* s)
		{
			s;
		}
	}


	return 0;
}
