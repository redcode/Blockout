/* Blockout Kit C API - Blockout.h
   ____   ___		  ___		      ___
  /  _ ) /  /____  _____ /  /__ ____  __ ___ /	/_
 /  _  \/  //  _ \/   _//    _//  _ \/	/  //  __/
/______/\__/\____/\____/__/\__/\____/\___,_/\__/
Copyright (C) 2013-2018 Manuel Sainz de Baranda y Goñi.
Released under the terms of the GNU Lesser General Public License v3. */

#ifndef __games_puzzle_Blockout_H__
#define __games_puzzle_Blockout_H__

#include <Z/types/base.h>
#include <Z/keys/status.h>

#ifndef BLOCKOUT_API
#	ifdef BLOCKOUT_STATIC
#		define BLOCKOUT_API
#	else
#		define BLOCKOUT_API Z_API
#	endif
#endif

#define BLOCKOUT_MINIMUM_SIZE_X	5
#define BLOCKOUT_MINIMUM_SIZE_Y	5
#define BLOCKOUT_MINIMUM_SIZE_Z	10

#define BLOCKOUT_PIECE_SET_BASIC    1
#define BLOCKOUT_PIECE_SET_FLAT     2
#define BLOCKOUT_PIECE_SET_EXTENDED 0

#define BLOCKOUT_PIECE_COUNT_BASIC    7
#define BLOCKOUT_PIECE_COUNT_FLAT     8
#define BLOCKOUT_PIECE_COUNT_EXTENDED 41

Z_DEFINE_STRICT_UNION (
	zuint16 value;

	struct {Z_ENDIANIZED_MEMBERS(16, 2) (
		struct {Z_BIT_FIELD(8, 7) (
			zuint8 has	  :2,
			zuint8 x_positive :1,
			zuint8 x_negative :1,
			zuint8 y_positive :1,
			zuint8 y_negative :1,
			zuint8 z_positive :1,
			zuint8 z_negative :1
		)} faces,

		zuint8 piece_index
	)} fields;
) BlockoutCell;

#define BLOCKOUT_CELL_MASK_SOLID	   Z_UINT16(0x4000)
#define BLOCKOUT_CELL_MASK_X_POSITIVE_FACE Z_UINT16(0x2000)
#define BLOCKOUT_CELL_MASK_X_NEGATIVE_FACE Z_UINT16(0x1000)
#define BLOCKOUT_CELL_MASK_Y_POSITIVE_FACE Z_UINT16(0x0800)
#define BLOCKOUT_CELL_MASK_Y_NEGATIVE_FACE Z_UINT16(0x0400)
#define BLOCKOUT_CELL_MASK_Z_POSITIVE_FACE Z_UINT16(0x0200)
#define BLOCKOUT_CELL_MASK_Z_NEGATIVE_FACE Z_UINT16(0x0100)
#define BLOCKOUT_CELL_MASK_PIECE_INDEX	   Z_UINT16(0x003F)

typedef struct {
	BlockoutCell matrix[5][5][5];
	Z3DSInt8     a, b;
} BlockoutPiece;

typedef struct {
	BlockoutCell*  matrix;
	Z3DSInt8       size;
	zsint8	       top;
	BlockoutPiece* piece;
	BlockoutPiece* next_piece;
	Z3DSInt8       piece_point;
	zsint8	       full_plane_indices[5];
	zuint8	       full_plane_count;
	zuint8	       piece_index;
	zuint8	       next_piece_index;
	BlockoutPiece  pieces[2];
} Blockout;

typedef zuint8 BlockoutResult;

#define BLOCKOUT_RESULT_HIT	     1
#define BLOCKOUT_RESULT_CONSOLIDATED 2

Z_C_SYMBOLS_BEGIN

BLOCKOUT_API void	    blockout_piece_build       (BlockoutPiece* piece,
							zuint8	       piece_set,
							zuint8	       piece_index);

BLOCKOUT_API void	    blockout_initialize	       (Blockout*      object);

BLOCKOUT_API void	    blockout_finalize	       (Blockout*      object);

BLOCKOUT_API ZStatus	    blockout_prepare	       (Blockout*      object,
							Z3DSInt8       size,
							zuint8	       next_piece_set,
							zuint8	       next_piece_index);

BLOCKOUT_API zboolean	    blockout_insert_piece      (Blockout*      object,
							zuint8	       next_piece_set,
							zuint8	       next_piece_index);

BLOCKOUT_API BlockoutResult blockout_move_piece	       (Blockout*      object,
							Z3DSInt8       movement);

BLOCKOUT_API BlockoutResult blockout_rotate_piece      (Blockout*      object,
							Z3DSInt8       rotation);

BLOCKOUT_API void	    blockout_drop_piece	       (Blockout*      object);

BLOCKOUT_API void	    blockout_remove_full_planes(Blockout*      object);

Z_C_SYMBOLS_END

#endif /* __games_puzzle_Blockout_H__ */
