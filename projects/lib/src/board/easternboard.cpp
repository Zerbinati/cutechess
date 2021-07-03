/*
    This file is part of Cute Chess.

    Cute Chess is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cute Chess is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cute Chess.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "easternboard.h"
#include <QStringList>
#include "easternzobrist.h"
#include "boardtransition.h"

namespace Chess {

EasternBoard::EasternBoard(EasternZobrist* zobrist)
    : Board(zobrist),
      m_arwidth(0),
      m_sign(1),
      m_plyOffset(0),
      m_reversibleMoveCount(0),
      m_kingCanCapture(true),
      m_hasCastling(false),
      m_pawnAmbiguous(false),
      m_multiDigitNotation(false),
      m_zobrist(zobrist)
{
    setPieceType(King,   tr("king"),   "K",KingMovement,   "XQK");
    setPieceType(Advisor,tr("advisor"),"A",AdvisorMovement,"XQA");
    setPieceType(Bishop, tr("bishop"), "B",BishopMovement, "XQE");
    setPieceType(Knight, tr("knight"), "N",KnightMovement, "XQN");
    setPieceType(Rook,   tr("rook"),   "R",RookMovement,   "XQR");
    setPieceType(Cannon, tr("cannon"), "C",CannonMovement, "XQC");
    setPieceType(Pawn,   tr("pawn"),   "P",PawnMovement,   "XQP");

    m_pawnSteps += {CaptureStep, -1};
    m_pawnSteps += {FreeStep, 0};
    m_pawnSteps += {CaptureStep, 1};
}

int EasternBoard::width() const
{
    return 9;
}

int EasternBoard::height() const
{
    return 10;
}

bool EasternBoard::kingsCountAssertion(int whiteKings, int blackKings) const
{
    return whiteKings == 1 && blackKings == 1;
}

bool EasternBoard::kingCanCapture() const
{
    return true;
}

bool EasternBoard::hasCastling() const
{
    return false;
}

bool EasternBoard::variantHasChanneling(Side, int) const
{
    return false;
}

void EasternBoard::vInitialize()
{
    m_kingCanCapture = kingCanCapture();
    m_hasCastling = hasCastling();

    m_arwidth = width() + 2;

    m_castlingRights.rookSquare[Side::White][QueenSide] = 0;
    m_castlingRights.rookSquare[Side::White][KingSide] = 0;
    m_castlingRights.rookSquare[Side::Black][QueenSide] = 0;
    m_castlingRights.rookSquare[Side::Black][KingSide] = 0;

    m_kingSquare[Side::White] = 0;
    m_kingSquare[Side::Black] = 0;

    m_castleTarget[Side::White][QueenSide] = (height() + 1) * m_arwidth + 1 + castlingFile(QueenSide);
    m_castleTarget[Side::White][KingSide] = (height() + 1) * m_arwidth + 1 + castlingFile(KingSide);
    m_castleTarget[Side::Black][QueenSide] = 2 * m_arwidth + 1 + castlingFile(QueenSide);
    m_castleTarget[Side::Black][KingSide] = 2 * m_arwidth + 1 + castlingFile(KingSide);

    m_pawnPreOffsets.resize(3);
    m_pawnPreOffsets[0] = m_arwidth;
    m_pawnPreOffsets[1] = -1;
    m_pawnPreOffsets[2] = 1;

    m_knightObstacleOffsets.resize(4);
    m_knightObstacleOffsets[0] = -m_arwidth;
    m_knightObstacleOffsets[1] = -1;
    m_knightObstacleOffsets[2] = 1;
    m_knightObstacleOffsets[3] = m_arwidth;
    m_knightPreOffsets.resize(8);
    m_knightPreOffsets[0] = -2 * m_arwidth - 1;
    m_knightPreOffsets[1] = -2 * m_arwidth + 1;
    m_knightPreOffsets[2] = -m_arwidth - 2;
    m_knightPreOffsets[3] = m_arwidth - 2;
    m_knightPreOffsets[4] = -m_arwidth + 2;
    m_knightPreOffsets[5] = m_arwidth + 2;
    m_knightPreOffsets[6] = 2 * m_arwidth - 1;
    m_knightPreOffsets[7] = 2 * m_arwidth + 1;

    m_bishopObstacleOffsets.resize(4);
    m_bishopObstacleOffsets[0] = -m_arwidth - 1;
    m_bishopObstacleOffsets[1] = -m_arwidth + 1;
    m_bishopObstacleOffsets[2] = m_arwidth - 1;
    m_bishopObstacleOffsets[3] = m_arwidth + 1;
    m_bishopPreOffsets.resize(4);
    m_bishopPreOffsets[0] = -2 * m_arwidth - 2;
    m_bishopPreOffsets[1] = -2 * m_arwidth + 2;
    m_bishopPreOffsets[2] = 2 * m_arwidth - 2;
    m_bishopPreOffsets[3] = 2 * m_arwidth + 2;

    m_rookOffsets.resize(4);
    m_rookOffsets[0] = -m_arwidth;
    m_rookOffsets[1] = -1;
    m_rookOffsets[2] = 1;
    m_rookOffsets[3] = m_arwidth;

    m_advisorOffsets.resize(4);
    m_advisorOffsets[0] = -m_arwidth - 1;
    m_advisorOffsets[1] = -m_arwidth + 1;
    m_advisorOffsets[2] = m_arwidth - 1;
    m_advisorOffsets[3] = m_arwidth + 1;

    m_kingMeetOffsets.resize(2);
    m_kingMeetOffsets[0] = -m_arwidth;
    m_kingMeetOffsets[1] = m_arwidth;

    m_pawnAmbiguous = false;
    m_multiDigitNotation =  (height() > 9 && coordinateSystem() == NormalCoordinates)
                 || (width() > 9 && coordinateSystem() == InvertedCoordinates);
}

bool EasternBoard::inFort(int square) const
{
    bool retVal;
    Square sq = chessSquare(square);
    if(((sq.file() > 2) && (sq.file() < 6)) &&
      (((sq.rank() >= 0) && (sq.rank() < 3)) ||
       ((sq.rank() > 6) && (sq.rank() < 10))))
    {
        retVal = true;
    }
    else
    {
        retVal = false;
    }

    return retVal;
}

QString EasternBoard::sanMoveString(const Move &move)
{
    QString str;
    int source = move.sourceSquare();
    int target = move.targetSquare();
    Piece piece = pieceAt(source);
    Piece capture = pieceAt(target);
    Square square = chessSquare(source);

    if (source == target)
        capture = Piece::NoPiece;

    char checkOrMate = 0;
    makeMove(move);
    if (inCheck(sideToMove()))
    {
        if (canMove())
            checkOrMate = '+';
        else
            checkOrMate = '#';
    }
    undoMove();

    // drop move
    if (source == 0 && move.promotion() != Piece::NoPiece)
    {
        str = lanMoveString(move);
        if (checkOrMate != 0)
            str += checkOrMate;
        return str;
    }

    bool needRank = false;
    bool needFile = false;

    if (piece.type() == Pawn)
    {
        if (m_pawnAmbiguous)
        {
            needFile = true;
            needRank = true; // for Xboard-compatibility
        }
        if (capture.isValid())
            needFile = true;
    }
    else if (piece.type() == King)
    {
        CastlingSide cside = castlingSide(move);
        if (cside != NoCastlingSide)
        {
            if (cside == QueenSide)
                str = "O-O-O";
            else
                str = "O-O";
            if (checkOrMate != 0)
                str += checkOrMate;
            return str;
        }
    }
    if (piece.type() != Pawn)	// not pawn
    {
        str += pieceSymbol(piece).toUpper();
        QVarLengthArray<Move> moves;
        generateMoves(moves, piece.type());

        for (int i = 0; i < moves.size(); i++)
        {
            const Move& move2 = moves[i];
            if (move2.sourceSquare() == 0
            ||  move2.sourceSquare() == source
            ||  move2.targetSquare() != target)
                continue;

            if (!vIsLegalMove(move2))
                continue;

            Square square2(chessSquare(move2.sourceSquare()));
            if (square2.file() != square.file())
                needFile = true;
            else if (square2.rank() != square.rank())
                needRank = true;
        }
    }
    if (needFile)
        str += 'a' + square.file();
    if (needRank)
        str += QString::number(1 + square.rank());

    if (capture.isValid())
        str += 'x';

    str += squareString(target);

    if (move.promotion() != Piece::NoPiece)
        str += "=" + pieceSymbol(move.promotion()).toUpper();

    if (checkOrMate != 0)
        str += checkOrMate;

    return str;
}

Move EasternBoard::moveFromLanString(const QString& str)
{
    Move move(Board::moveFromLanString(str));

    Side side = sideToMove();
    int source = move.sourceSquare();
    int target = move.targetSquare();

    if (source == m_kingSquare[side]
    &&  qAbs(source - target) != 1)
    {
        const int* rookSq = m_castlingRights.rookSquare[side];
        if (target == m_castleTarget[side][QueenSide])
            target = rookSq[QueenSide];
        else if (target == m_castleTarget[side][KingSide])
            target = rookSq[KingSide];

        if (target != 0)
            return Move(source, target);
    }

    return move;
}

Move EasternBoard::moveFromSanString(const QString& str)
{
    if (str.length() < 2)
        return Move();

    QString mstr = str;
    Side side = sideToMove();

    // Ignore check/mate/strong move/blunder notation
    while (mstr.endsWith('+') || mstr.endsWith('#')
    ||     mstr.endsWith('!') || mstr.endsWith('?'))
    {
        mstr.chop(1);
    }

    if (mstr.length() < 2)
        return Move();

    // Castling
    if (mstr.startsWith("O-O"))
    {
        CastlingSide cside;
        if (mstr == "O-O")
            cside = KingSide;
        else if (mstr == "O-O-O")
            cside = QueenSide;
        else
            return Move();

        int source = m_kingSquare[side];
        int target = m_castlingRights.rookSquare[side][cside];

        Move move(source, target);
        if (isLegalMove(move))
            return move;
        else
            return Move();
    }

    // number of digits in notation of squares
    int digits = 1;

    // only for tall boards: find maximum number of sequential digits
    if (m_multiDigitNotation)
    {
        int count = 0;
        for (const QChar& ch : qAsConst(mstr))
        {
            if (ch.isDigit())
            {
                count++;
                if (count > digits)
                    digits = count;
            }
            else
                count = 0;
        }
    }

    Square sourceSq;
    Square targetSq;
    QString::const_iterator it = mstr.cbegin();

    // A SAN move can't start with the capture mark, and
    if (*it == 'x')
        return Move();
    // a pawn move should not specify the piece type
    if (pieceFromSymbol(*it) == Pawn)
        it++; // ignore character
    // Piece type
    Piece piece = pieceFromSymbol(*it);
    if (piece.side() != Side::White)
        piece = Piece::NoPiece;
    else
        piece.setSide(side);
    if (piece.isEmpty())
    {
        piece = Piece(side, Pawn);
        targetSq = chessSquare(mstr.mid(0, 1 + digits));
        if (isValidSquare(targetSq))
            it += 1 + digits;
    }
    else
    {
        ++it;

        // Drop moves
        if (*it == '@')
        {
            targetSq = chessSquare(mstr.right(1 + digits));
            if (!isValidSquare(targetSq))
                return Move();

            Move move(0, squareIndex(targetSq), piece.type());
            if (isLegalMove(move))
                return move;
            return Move();
        }
    }

    bool stringIsCapture = false;

    if (!isValidSquare(targetSq))
    {
        // Source square's file
        sourceSq.setFile(it->toLatin1() - 'a');
        if (sourceSq.file() < 0 || sourceSq.file() >= width())
            sourceSq.setFile(-1);
        else if (++it == mstr.cend())
            return Move();

        // Source square's rank
        if (it->isDigit())
        {
            const QString tmp(mstr.mid(it - mstr.constBegin(),
                           digits));
            sourceSq.setRank(-1 + tmp.toInt());
            if (sourceSq.rank() < 0 || sourceSq.rank() >= height())
                return Move();
            it += digits;
        }
        if (it == mstr.cend())
        {
            // What we thought was the source square, was
            // actually the target square.
            if (isValidSquare(sourceSq))
            {
                targetSq = sourceSq;
                sourceSq.setRank(-1);
                sourceSq.setFile(-1);
            }
            else
                return Move();
        }
        // Capture
        else if (*it == 'x')
        {
            if(++it == mstr.cend())
                return Move();
            stringIsCapture = true;
        }

        // Target square
        if (!isValidSquare(targetSq))
        {
            if (it + 1 >= mstr.cend())
                return Move();
            QString tmp(mstr.mid(it - mstr.cbegin(), 1 + digits));
            targetSq = chessSquare(tmp);
            it += tmp.size();
        }
    }
    if (!isValidSquare(targetSq))
        return Move();
    int target = squareIndex(targetSq);

    // Make sure that the move string is right about whether
    // or not the move is a capture.
    bool isCapture = false;
    if (pieceAt(target).side() == side.opposite())
        isCapture = true;
    if (isCapture != stringIsCapture)
        return Move();

    // Promotion
    int promotion = Piece::NoPiece;
    if (it != mstr.cend())
    {
        if ((*it == '=' || *it == '(') && ++it == mstr.cend())
            return Move();

        promotion = pieceFromSymbol(*it).type();
        if (promotion == Piece::NoPiece)
            return Move();
    }

    QVarLengthArray<Move> moves;
    generateMoves(moves, piece.type());
    const Move* match = nullptr;

    // Loop through all legal moves to find a move that matches
    // the data we got from the move string.
    for (int i = 0; i < moves.size(); i++)
    {
        const Move& move = moves[i];
        if (move.sourceSquare() == 0 || move.targetSquare() != target)
            continue;
        Square sourceSq2 = chessSquare(move.sourceSquare());
        if (sourceSq.rank() != -1 && sourceSq2.rank() != sourceSq.rank())
            continue;
        if (sourceSq.file() != -1 && sourceSq2.file() != sourceSq.file())
            continue;
        // Castling moves were handled earlier
        if (pieceAt(target) == Piece(side, Rook))
            continue;
        if (move.promotion() != promotion)
            continue;

        if (!vIsLegalMove(move))
            continue;

        // Return an empty move if there are multiple moves that
        // match the move string.
        if (match != nullptr)
            return Move();
        match = &move;
    }

    if (match != nullptr)
        return *match;

    return Move();
}

QString EasternBoard::castlingRightsString(FenNotation notation) const
{
    QString str;

    for (int side = Side::White; side <= Side::Black; side++)
    {
        for (int cside = KingSide; cside >= QueenSide; cside--)
        {
            int rs = m_castlingRights.rookSquare[side][cside];
            if (rs == 0)
                continue;

            int offset = (cside == QueenSide) ? -1: 1;
            Piece piece;
            int i = rs + offset;
            bool ambiguous = false;

            // If the castling rook is not the outernmost rook,
            // the castling square is ambiguous
            while (!(piece = pieceAt(i)).isWall())
            {
                if (piece == Piece(Side::Type(side), Rook))
                {
                    ambiguous = true;
                    break;
                }
                i += offset;
            }

            QChar c;
            // If the castling square is ambiguous, then we can't
            // use 'K' or 'Q'. Instead we'll use the square's file.
            if (ambiguous || notation == ShredderFen)
                c = QChar('a' + chessSquare(rs).file());
            else
            {
                if (cside == 0)
                    c = 'q';
                else
                    c = 'k';
            }
            if (side == upperCaseSide())
                c = c.toUpper();
            str += c;
        }
    }

    if (str.length() == 0)
        str = "-";
    return str;
}

int EasternBoard::pawnAmbiguity(StepType t) const
{
    Q_UNUSED(t);
    int count = 0;
    //TODO: add the multiple pawn on the same file check
    return count;
}

QString EasternBoard::vFenIncludeString(FenNotation notation) const
{
    Q_UNUSED(notation);
    return "";
}

QString EasternBoard::vFenString(FenNotation notation) const
{
    // Castling rights
    QString fen = castlingRightsString(notation) + ' ';

    fen += '-';

    fen +=vFenIncludeString(notation);

    // Reversible halfmove count
    fen += ' ';
    fen += QString::number(m_reversibleMoveCount);

    // Full move number
    fen += ' ';
    fen += QString::number((m_history.size() + m_plyOffset) / 2 + 1);

    return fen;
}

bool EasternBoard::parseCastlingRights(QChar c)
{
    if (!m_hasCastling)
        return false;

    int offset = 0;
    CastlingSide cside = NoCastlingSide;
    Side side = (c.isUpper()) ? upperCaseSide() : upperCaseSide().opposite();
    c = c.toLower();

    if (c == 'q')
    {
        cside = QueenSide;
        offset = -1;
    }
    else if (c == 'k')
    {
        cside = KingSide;
        offset = 1;
    }

    int kingSq = m_kingSquare[side];

    if (offset != 0)
    {
        Piece piece;
        int i = kingSq + offset;
        int rookSq = 0;

        // Locate the outernmost rook on the castling side
        while (!(piece = pieceAt(i)).isWall())
        {
            if (piece == Piece(side, Rook))
                rookSq = i;
            i += offset;
        }
        if (rookSq != 0)
        {
            setCastlingSquare(side, cside, rookSq);
            return true;
        }
    }
    else	// Shredder FEN or X-FEN
    {
        int file = c.toLatin1() - 'a';
        if (file < 0 || file >= width())
            return false;

        // Get the rook's source square
        int rookSq;
        if (side == Side::White)
            rookSq = (height() + 1) * m_arwidth + 1 + file;
        else
            rookSq = 2 * m_arwidth + 1 + file;

        // Make sure the king and the rook are on the same rank
        if (abs(kingSq - rookSq) >= width())
            return false;

        // Update castling rights in the FenData object
        if (pieceAt(rookSq) == Piece(side, Rook))
        {
            if (rookSq > kingSq)
                cside = KingSide;
            else
                cside = QueenSide;
            setCastlingSquare(side, cside, rookSq);
            return true;
        }
    }

    return false;
}

bool EasternBoard::vSetFenString(const QStringList& fen)
{
    //qWarning("set EasternBoard fen string!");
    if (fen.size() < 2)
        return false;
    QStringList::const_iterator token = fen.begin();

    // Find the king squares
    int kingCount[2] = {0, 0};
    for (int sq = 0; sq < arraySize(); sq++)
    {
        Piece tmp = pieceAt(sq);
        if (tmp.type() == King)
        {
            m_kingSquare[tmp.side()] = sq;
            kingCount[tmp.side()]++;
        }
    }
    if (!kingsCountAssertion(kingCount[Side::White],
                 kingCount[Side::Black]))
        return false;

    // short non-standard format without castling and ep fields?
    bool isShortFormat = false;
    if (fen.size() < 3)
        token->toInt(&isShortFormat);

    // allowed only for variants without castling and en passant captures
    if (isShortFormat && m_hasCastling)
        return false;

    // Castling rights
    m_castlingRights.rookSquare[Side::White][QueenSide] = 0;
    m_castlingRights.rookSquare[Side::White][KingSide] = 0;
    m_castlingRights.rookSquare[Side::Black][QueenSide] = 0;
    m_castlingRights.rookSquare[Side::Black][KingSide] = 0;

    if (!isShortFormat)
    {
        if (*token != "-")
        {
            QString::const_iterator c;
            for (c = token->begin(); c != token->end(); ++c)
            {
                if (!parseCastlingRights(*c))
                    return false;
            }
        }
        ++token;
    }

    Side side(sideToMove());
    m_sign = (side == Side::White) ? 1 : -1;

    if (!isShortFormat)
        ++token;

    // Reversible halfmove count
    if (token != fen.end())
    {
        bool ok;
        int tmp = token->toInt(&ok);
        if (!ok || tmp < 0)
            return false;
        m_reversibleMoveCount = tmp;
        ++token;
    }
    else
        m_reversibleMoveCount = 0;

    // Read the full move number and calculate m_plyOffset
    if (token != fen.end())
    {
        bool ok;
        int tmp = token->toInt(&ok);
        if (!ok || tmp < 1)
            return false;
        m_plyOffset = 2 * (tmp - 1);
    }
    else
        m_plyOffset = 0;

    if (m_sign != 1)
        m_plyOffset++;

    m_history.clear();
    return true;
}

int EasternBoard::captureType(const Move& move) const
{
    return Board::captureType(move);
}

EasternBoard::CastlingSide EasternBoard::castlingSide(const Move& move) const
{
    int target = move.targetSquare();
    const int* rookSq = m_castlingRights.rookSquare[sideToMove()];
    if (target == rookSq[QueenSide])
        return QueenSide;
    if (target == rookSq[KingSide])
        return KingSide;
    return NoCastlingSide;
}

QString EasternBoard::lanMoveString(const Move &move)
{
    CastlingSide cside = castlingSide(move);
    if (cside != NoCastlingSide && !isRandomVariant())
    {
        Move tmp(move.sourceSquare(),
             m_castleTarget[sideToMove()][cside]);
        return Board::lanMoveString(tmp);
    }

    return Board::lanMoveString(move);
}

void EasternBoard::setCastlingSquare(Side side,
                     CastlingSide cside,
                     int square)
{
    int& rs = m_castlingRights.rookSquare[side][cside];
    if (rs == square)
        return;
    if (rs != 0)
        xorKey(m_zobrist->castling(side, rs));
    if (square != 0)
        xorKey(m_zobrist->castling(side, square));
    rs = square;
}

void EasternBoard::removeCastlingRights(int square)
{
    Piece piece = pieceAt(square);
    if (piece.type() != Rook)
        return;

    Side side(piece.side());
    const int* cr = m_castlingRights.rookSquare[side];

    if (square == cr[QueenSide])
        setCastlingSquare(side, QueenSide, 0);
    else if (square == cr[KingSide])
        setCastlingSquare(side, KingSide, 0);
}

void EasternBoard::removeCastlingRights(Side side)
{
    setCastlingSquare(side, QueenSide, 0);
    setCastlingSquare(side, KingSide, 0);
}

int EasternBoard::castlingFile(CastlingSide castlingSide) const
{
    Q_ASSERT(castlingSide != NoCastlingSide);
    return castlingSide == QueenSide ? 2 : width() - 2; // usually C and G
}

void EasternBoard::vMakeMove(const Move& move, BoardTransition* transition)
{
    Side side = sideToMove();
    int source = move.sourceSquare();
    int target = move.targetSquare();
    Piece capture = pieceAt(target);
    int promotionType = move.promotion();
    int pieceType = pieceAt(source).type();
    int* rookSq = m_castlingRights.rookSquare[side];
    bool clearSource = true;
    bool isReversible = true;

    Q_ASSERT(target != 0);

    MoveData md = { capture, m_castlingRights,
            NoCastlingSide, m_reversibleMoveCount };

    if (source == 0)
    {
        Q_ASSERT(promotionType != Piece::NoPiece);

        pieceType = promotionType;
        promotionType = Piece::NoPiece;
        clearSource = false;
        isReversible = false;
    }

    if (source == target)
        clearSource = 0;

    if (pieceType == King)
    {
        // In case of a castling move, make the rook's move
        CastlingSide cside = castlingSide(move);
        if (cside != NoCastlingSide)
        {
            md.castlingSide = cside;
            int rookSource = target;
            target = m_castleTarget[side][cside];
            int rookTarget = (cside == QueenSide) ? target + 1 : target -1;
            if (rookTarget == source || target == source)
                clearSource = false;

            Piece rook = Piece(side, Rook);
            setSquare(rookSource, Piece::NoPiece);
            setSquare(rookTarget, rook);
            // FIDE rules 5.2, 9.3, PGN/FEN spec. 16.1.3.5:
            // 50-moves counting goes on when castling.

            if (transition != nullptr)
                transition->addMove(chessSquare(rookSource),
                            chessSquare(rookTarget));
        }
        m_kingSquare[side] = target;
        // Any king move removes all castling rights
        setCastlingSquare(side, QueenSide, 0);
        setCastlingSquare(side, KingSide, 0);
    }
    else if (pieceType == Rook)
    {
        // Remove castling rights from the rook's square
        for (int i = QueenSide; i <= KingSide; i++)
        {
            if (source == rookSq[i])
            {
                setCastlingSquare(side, CastlingSide(i), 0);
                isReversible = false;
                break;
            }
        }
    }

    if (captureType(move) != Piece::NoPiece)
    {
        removeCastlingRights(target);
        isReversible = false;
    }

    if (promotionType != Piece::NoPiece)
        isReversible = false;

    if (transition != nullptr)
    {
        if (source != 0)
            transition->addMove(chessSquare(source),
                        chessSquare(target));
        else
            transition->addDrop(Piece(side, pieceType),
                        chessSquare(target));
    }

    setSquare(target, Piece(side, pieceType));
    if (clearSource)
        setSquare(source, Piece::NoPiece);

    if (isReversible)
        m_reversibleMoveCount++;
    else
        m_reversibleMoveCount = 0;

    m_history.append(md);
    m_sign *= -1;
}

void EasternBoard::vUndoMove(const Move& move)
{
    const MoveData& md = m_history.last();
    int source = move.sourceSquare();
    int target = move.targetSquare();

    m_sign *= -1;
    Side side = sideToMove();

    m_reversibleMoveCount = md.reversibleMoveCount;
    m_castlingRights = md.castlingRights;

    CastlingSide cside = md.castlingSide;
    if (cside != NoCastlingSide)
    {
        m_kingSquare[side] = source;
        // Move the rook back after castling
        int tmp = m_castleTarget[side][cside];
        setSquare(tmp, Piece::NoPiece);
        tmp = (cside == QueenSide) ? tmp + 1 : tmp - 1;
        setSquare(tmp, Piece::NoPiece);

        setSquare(target, Piece(side, Rook));
        setSquare(source, Piece(side, King));
        m_history.pop_back();
        return;
    }
    else if (target == m_kingSquare[side])
    {
        m_kingSquare[side] = source;
    }

    if (move.promotion() != Piece::NoPiece)
    {
        if (source != 0)
        {
            if (variantHasChanneling(side, source))
                setSquare(source, pieceAt(target));
            else
                setSquare(source, Piece(side, Pawn));
        }
    }
    else
        setSquare(source, pieceAt(target));

    setSquare(target, md.capture);
    m_history.pop_back();
}

void EasternBoard::generateMovesForPiece(QVarLengthArray<Move>& moves,
                     int pieceType,
                     int square) const
{
    if(pieceHasMovement(pieceType, PawnMovement))
        generatePawnMoves(square, moves);
    if(pieceHasMovement(pieceType, KingMovement))
        generateKingMoves(square, moves);
    if (pieceHasMovement(pieceType, KnightMovement))
        generateKnightMoves(square, moves);
    if (pieceHasMovement(pieceType, BishopMovement))
        generateBishopMoves(square, moves);
    if (pieceHasMovement(pieceType, RookMovement))
        generateRookMoves(square, moves);
    if (pieceHasMovement(pieceType, AdvisorMovement))
        generateAdvisorMoves(square, moves);
    if (pieceHasMovement(pieceType, CannonMovement))
        generateCannonMoves(square, moves);
}

bool EasternBoard::inCheck(Side side, int square) const
{
    Side opSide = side.opposite();
    if (square == 0)
    {
        square = m_kingSquare[side];
        // In the "horde" variant the horde side has no king
        if (square == 0)
            return false;
    }

    Piece oppKing(opSide, King);
    Piece ownKing(side, King);
    Piece piece;

    // Cannons attacks
    for (int i = 0; i < m_rookOffsets.size(); i++)
    {
        int offset = m_rookOffsets[i];
        int targetSquare = square + offset;

        Piece capture;
        int obstacle = 0;
        while(!(capture = pieceAt(targetSquare)).isWall())
        {
            if(!capture.isEmpty())
            {
                obstacle++;
                if(obstacle == 2 && capture.side() == opSide
                   &&  pieceHasMovement(capture.type(), CannonMovement))
                {
                    return true;
                }
            }
            targetSquare += offset;
        }
    }

    // Knight, archbishop, chancellor attacks
    for (int i = 0; i < m_knightPreOffsets.size(); i++)
    {
        int checkSquare = square + m_knightPreOffsets[i];
        piece = pieceAt(checkSquare);
        if (piece.side() == opSide &&
            pieceHasMovement(piece.type(), KnightMovement))
        {
            QVarLengthArray<int> knightRelOffsets;
            knightRelOffsets.clear();
            for (int i = 0; i < m_knightObstacleOffsets.size(); i++)
            {
                int obstacleSquare = checkSquare + m_knightObstacleOffsets[i];
                if (isValidSquare(chessSquare(obstacleSquare)))
                {
                    Piece obstaclePiece = pieceAt(obstacleSquare);
                    if(obstaclePiece.isEmpty())
                    {
                        knightRelOffsets.append(m_knightPreOffsets[2*i]);
                        knightRelOffsets.append(m_knightPreOffsets[2*i+1]);
                    }
                }
            }
            for (int i = 0; i < knightRelOffsets.size(); i++)
            {
                piece = pieceAt(checkSquare + knightRelOffsets[i]);
                if (m_kingCanCapture &&  piece == ownKing)
                {
                    return true;
                }
            }
        }
    }

    // Rook attacks
    for (int i = 0; i < m_rookOffsets.size(); i++)
    {
        int offset = m_rookOffsets[i];
        int targetSquare = square + offset;
        if (m_kingCanCapture
        &&  pieceAt(targetSquare) == oppKing)
            return true;
        while ((piece = pieceAt(targetSquare)).isEmpty()
        ||     piece.side() == opSide)
        {
            if (!piece.isEmpty())
            {
                if (pieceHasMovement(piece.type(), RookMovement))
                    return true;
                break;
            }
            targetSquare += offset;
        }
    }

    // Pawn attacks
    QVarLengthArray<int> pawnAttackOffsets;
    pawnAttackOffsets.clear();
    int p_sign = (side == Side::Black) ? 1: -1;
    int attackStep = p_sign * m_arwidth;

    pawnAttackOffsets.append(attackStep);
    pawnAttackOffsets.append(m_pawnPreOffsets[1]);
    pawnAttackOffsets.append(m_pawnPreOffsets[2]);
    for (int i = 0; i < pawnAttackOffsets.size(); i++)
    {
        piece = pieceAt(square + pawnAttackOffsets[i]);
        if (piece.side() == opSide
        &&  pieceHasMovement(piece.type(), PawnMovement))
            return true;
    }

    // King meet Attack (in the basic xiangqi rule, the two opposite
    // king are not allowed to face to face
    for (int i = 0; i < m_kingMeetOffsets.size(); i++)
    {
        int offset = m_kingMeetOffsets[i];
        int targetSquare = square + offset;

        Piece capture;
        int obstacle = 0;
        while(!(capture = pieceAt(targetSquare)).isWall())
        {
            if(!capture.isEmpty())
            {
                obstacle++;
                if(obstacle == 1 && capture.side() == opSide
                   && capture.type() == King)
                {
                    return true;
                }
            }
            targetSquare += offset;
        }
    }

    return false;
}

bool EasternBoard::isLegalPosition()
{
    Side side = sideToMove().opposite();
    if (inCheck(side))
        return false;

    if (m_history.isEmpty())
        return true;

    const Move& move = lastMove();

    // Make sure that no square between the king's initial and final
    // squares (including the initial and final squares) are under
    // attack (in check) by the opponent.
    CastlingSide cside = m_history.last().castlingSide;
    if (cside != NoCastlingSide)
    {
        int source = move.sourceSquare();
        int target = m_castleTarget[side][cside];
        int offset = (source <= target) ? 1 : -1;

        if (source == target)
        {
            offset = (cside == KingSide) ? 1 : -1;
            int i = target - offset;
            for (;;)
            {
                i -= offset;
                Piece piece(pieceAt(i));

                if (piece.isWall() || piece.side() == side)
                    return true;
                if (piece.side() == sideToMove()
                &&  pieceHasMovement(piece.type(), RookMovement))
                    return false;
            }
        }

        for (int i = source; i != target; i += offset)
        {
            if (inCheck(side, i))
                return false;
        }
    }

    return true;
}

bool EasternBoard::vIsLegalMove(const Move& move)
{
    Q_ASSERT(!move.isNull());

    if (!m_kingCanCapture
    &&  move.sourceSquare() == m_kingSquare[sideToMove()]
    &&  captureType(move) != Piece::NoPiece)
        return false;

    return Board::vIsLegalMove(move);
}

void EasternBoard::addPromotions(int sourceSquare,
                 int targetSquare,
                 QVarLengthArray<Move>& moves) const
{
    moves.append(Move(sourceSquare, targetSquare, Knight));
    moves.append(Move(sourceSquare, targetSquare, Bishop));
    moves.append(Move(sourceSquare, targetSquare, Rook));
    moves.append(Move(sourceSquare, targetSquare, Advisor));
}

void EasternBoard::generatePawnMoves(int sourceSquare,
                     QVarLengthArray<Move>& moves) const
{
    QVarLengthArray<int> pawnRelOffsets;
    pawnRelOffsets.clear();
    Side side(sideToMove());
    int p_sign = (side == Side::Black) ? 1: -1;
    int step = p_sign * m_arwidth;
    Square sq = chessSquare(sourceSquare);

    if((side == Side::White) && (sq.rank() < height()/2))
    {
        pawnRelOffsets.append(step);
    }
    else if((side == Side::White) && (sq.rank() > (height()/2 - 1)))
    {
        pawnRelOffsets.append(step);
        pawnRelOffsets.append(m_pawnPreOffsets[1]);
        pawnRelOffsets.append(m_pawnPreOffsets[2]);
    }
    if((side == Side::Black) && (sq.rank() > (height()/2 - 1)))
    {
        pawnRelOffsets.append(step);
    }
    else if((side == Side::Black) && (sq.rank() < height()/2))
    {
        pawnRelOffsets.append(step);
        pawnRelOffsets.append(m_pawnPreOffsets[1]);
        pawnRelOffsets.append(m_pawnPreOffsets[2]);
    }
    generateHoppingMoves(sourceSquare, pawnRelOffsets, moves);
}

void EasternBoard::generateCannonMoves(int sourceSquare,
                     QVarLengthArray<Move>& moves) const
{
    QVarLengthArray<int> cannonRelOffsets;
    cannonRelOffsets.clear();
    Side side = sideToMove();
    for (int i = 0; i < m_rookOffsets.size(); i++)
    {
        int offset = m_rookOffsets[i];
        int targetSquare = sourceSquare + offset;

        Piece capture;
        int obstacle = 0;
        while(!(capture = pieceAt(targetSquare)).isWall())
        {
            if(capture.isEmpty())
            {
                if(obstacle == 0)
                {
                    moves.append(Move(sourceSquare, targetSquare));
                }
            }
            else
            {
                obstacle++;
                if(obstacle == 2 && capture.side() != side)
                {
                    moves.append(Move(sourceSquare, targetSquare));
                    break;
                }
            }
            targetSquare += offset;
        }
    }
}

void EasternBoard::generateKnightMoves(int sourceSquare, QVarLengthArray<Move> &moves) const
{
    QVarLengthArray<int> knightRelOffsets;
    knightRelOffsets.clear();
    for (int i = 0; i < m_knightObstacleOffsets.size(); i++)
    {
        int obstacleSquare = sourceSquare + m_knightObstacleOffsets[i];
        if (isValidSquare(chessSquare(obstacleSquare)))
        {
            Piece obstaclePiece = pieceAt(obstacleSquare);
            if(obstaclePiece.isEmpty())
            {
                knightRelOffsets.append(m_knightPreOffsets[2*i]);
                knightRelOffsets.append(m_knightPreOffsets[2*i+1]);
            }
        }
    }
    generateHoppingMoves(sourceSquare, knightRelOffsets, moves);
}

void EasternBoard::generateBishopMoves(int sourceSquare, QVarLengthArray<Move> &moves) const
{
    QVarLengthArray<int> bishopRelOffsets;
    bishopRelOffsets.clear();
    Side side = sideToMove();
    for (int i = 0; i < m_bishopObstacleOffsets.size(); i++)
    {
        int obstacleSquare = sourceSquare + m_bishopObstacleOffsets[i];
        if (isValidSquare(chessSquare(obstacleSquare)))
        {
            Piece obstaclePiece = pieceAt(obstacleSquare);
            if(obstaclePiece.isEmpty())
            {
                Square sq = chessSquare(sourceSquare + m_bishopPreOffsets[i]);
                if(((side == Side::White) && (sq.rank() < 6)) ||
                   ((side == Side::Black) && (sq.rank() > 4)))
                {
                   bishopRelOffsets.append(m_bishopPreOffsets[i]);
                }
            }
        }
    }
    generateHoppingMoves(sourceSquare, bishopRelOffsets, moves);
}

void EasternBoard::generateAdvisorMoves(int sourceSquare, QVarLengthArray<Move> &moves) const
{
    QVarLengthArray<int> advisorRelOffsets;
    advisorRelOffsets.clear();
    for(int i = 0; i < m_advisorOffsets.size(); i++)
    {
        int targetSquare = sourceSquare + m_advisorOffsets[i];
        if(inFort(targetSquare))
        {
            advisorRelOffsets.append(m_advisorOffsets[i]);
        }
    }
    generateHoppingMoves(sourceSquare, advisorRelOffsets, moves);
}

void EasternBoard::generateKingMoves(int sourceSquare, QVarLengthArray<Move> &moves) const
{
    QVarLengthArray<int> kingRelOffsets;
    kingRelOffsets.clear();
    for(int i = 0; i < m_rookOffsets.size(); i++)
    {
        int targetSquare = sourceSquare + m_rookOffsets[i];
        if(inFort(targetSquare))
        {
            kingRelOffsets.append(m_rookOffsets[i]);
        }
    }
    generateHoppingMoves(sourceSquare, kingRelOffsets, moves);
}

void EasternBoard::generateRookMoves(int sourceSquare, QVarLengthArray<Move> &moves) const
{
    generateSlidingMoves(sourceSquare, m_rookOffsets, moves);
}

bool EasternBoard::canCastle(CastlingSide castlingSide) const
{
    Side side = sideToMove();
    int rookSq = m_castlingRights.rookSquare[side][castlingSide];
    if (rookSq == 0)
        return false;

    int kingSq = m_kingSquare[side];
    int target = m_castleTarget[side][castlingSide];
    int left;
    int right;
    int rtarget;

    // Find all the squares involved in the castling
    if (castlingSide == QueenSide)
    {
        rtarget = target + 1;

        if (target < rookSq)
            left = target;
        else
            left = rookSq;

        if (rtarget > kingSq)
            right = rtarget;
        else
            right = kingSq;
    }
    else	// Kingside
    {
        rtarget = target - 1;

        if (rtarget < kingSq)
            left = rtarget;
        else
            left = kingSq;

        if (target > rookSq)
            right = target;
        else
            right = rookSq;
    }

    // Make sure that the smallest back rank interval containing the king,
    // the castling rook, and their destination squares contains no pieces
    // other than the king and the castling rook.
    for (int i = left; i <= right; i++)
    {
        if (i != kingSq && i != rookSq && !pieceAt(i).isEmpty())
            return false;
    }

    return true;
}

void EasternBoard::generateCastlingMoves(QVarLengthArray<Move>& moves) const
{
    Side side = sideToMove();
    int source = m_kingSquare[side];
    for (int i = QueenSide; i <= KingSide; i++)
    {
        if (canCastle(CastlingSide(i)))
        {
            int target = m_castlingRights.rookSquare[side][i];
            moves.append(Move(source, target));
        }
    }
}

int EasternBoard::kingSquare(Side side) const
{
    Q_ASSERT(!side.isNull());
    return m_kingSquare[side];
}

bool EasternBoard::hasCastlingRight(Side side, CastlingSide castlingSide) const
{
    return m_castlingRights.rookSquare[side][castlingSide] != 0;
}

int EasternBoard::reversibleMoveCount() const
{
    return m_reversibleMoveCount;
}

Result EasternBoard::result()
{
    QString str;

    // Checkmate/Stalemate
    if (!canMove())
    {
        if (inCheck(sideToMove()))
        {
            Side winner = sideToMove().opposite();
            str = tr("%1 mates").arg(winner.toString());

            return Result(Result::Win, winner, str);
        }
        else
        {
            Side winner = sideToMove().opposite();
            str = tr("%1 Win by stalemate").arg(winner.toString());
            return Result(Result::Win, winner, str);
        }
    }

    // Insufficient mating material
    int material = 0;
    bool bishops[] = { false, false };
    for (int i = 0; i < arraySize(); i++)
    {
        const Piece& piece = pieceAt(i);
        if (!piece.isValid())
            continue;

        switch (piece.type())
        {
        case King:
            break;
        case Bishop:
        {
            auto color = chessSquare(i).color();
            if (color != Square::NoColor && !bishops[color])
            {
                material++;
                bishops[color] = true;
            }
            break;
        }
        case Advisor:
            material++;
            break;
        default:
            material += 2;
            break;
        }
    }
    if (material <= 1)
    {
        str = tr("Draw by insufficient mating material");
        return Result(Result::Draw, Side::NoSide, str);
    }

    // 50 move rule
    if (m_reversibleMoveCount >= 100)
    {
        str = tr("Draw by fifty moves rule");
        return Result(Result::Draw, Side::NoSide, str);
    }

    // 5-fold repetition
    if (repeatCount() >= 5)
    {
        str = tr("Draw by 5-fold repetition");
        return Result(Result::Draw, Side::NoSide, str);
    }

    return Result();
}

} // namespace Chess