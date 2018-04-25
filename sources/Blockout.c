/* Blockout Kit - Blockout.c
   ____   ___		  ___		      ___
  /  _ ) /  /____  _____ /  /__ ____  __ ___ /	/_
 /  _  \/  //  _ \/   _//    _//  _ \/	/  //  __/
/______/\__/\____/\____/__/\__/\____/\___,_/\__/
Copyright (C) 2013-2018 Manuel Sainz de Baranda y Goñi.
Released under the terms of the GNU Lesser General Public License v3. */

#include <Z/functions/base/Z3D.h>

#ifndef BLOCKOUT_STATIC
#	define BLOCKOUT_API Z_API_EXPORT
#endif

#ifdef BLOCKOUT_USE_LOCAL_HEADER
#	include "Blockout.h"
#else
#	include <games/puzzle/Blockout.h>
#endif

#ifdef BLOCKOUT_USE_C_STANDARD_LIBRARY
#	include <stdlib.h>
#	include <string.h>

#	define z_deallocate(block)			  free(block)
#	define z_reallocate(block, block_size)		  realloc(block, block_size)
#	define z_copy(block, block_size, output)	  memcpy(output, block, block_size)
#	define z_move(block, block_size, output)	  memmove(output, block, block_size)
#	define z_block_int8_set(block, block_size, value) memset(block, value, block_size)

	Z_INLINE zboolean plane_is_full(zuint16 const *cells, zusize cell_count)
		{
		for (; cell_count--; cells++) if (!*cells) return FALSE;
		return TRUE;
		}

#	define PLANE_IS_FULL plane_is_full
#else
#	include <ZBase/allocation.h>
#	include <ZBase/block.h>

#	define PLANE_IS_FULL(cells, cell_count)
		z_block_int16_find_value(cells, cell_count, 0) == NULL
#endif

#define HIT_BOUNDS 1
#define HIT_BOTTOM 2

#define C(value) Z_UINT32(0x##value) 

static zuint32 const polycubes[41] = {
	C(A8000001), C(98100003), C(98200007), C(8830000F), C(8840001F), C(9890000D),
	C(9890000F), C(98A0001D), C(98A0001E), C(98A00035), C(592001C9), C(59200079),
	C(5920019A), C(59200193), C(59200199), C(88B000D5), C(88B00075), C(98A00037),
	C(98A0003D), C(88B000B5), C(592000BA), C(94A20D60), C(54A20E90), C(94A20748),
	C(94A205C4), C(54A20AC8), C(94A20790), C(54A20B60), C(54A20368), C(94A20394),
	C(94A205D0), C(54A20AE0), C(549200B8), C(94920074), C(649200B2), C(651205C2),
	C(649200F2), C(651209C4), C(94A20354), C(54A203A8), C(A49200B5)
};

static zuint8 const polycube_indices[2][8] = {
	{5, 7, 8, 9, 32, 33, 34, 0}, /* Basic set */
	{0, 1, 2, 5,  6,  7,  8, 9}  /* Flat set  */
};


static void blockout_piece_update_faces(BlockoutPiece *piece)
	{
	BlockoutCell *cell;
	zuint8 x, y, z;

	for (z = piece->a.z; z <= piece->b.z; z++)
		for (y = piece->a.y; y <= piece->b.y; y++)
			for (x = piece->a.x; x <= piece->b.x; x++)
				if ((cell = &piece->matrix[z][y][x])->value)
		{
		if (x == piece->a.x || !piece->matrix[z][y][x - 1].value)
			cell->fields.faces.x_negative = TRUE;

		if (x == piece->b.x || !piece->matrix[z][y][x + 1].value)
			cell->fields.faces.x_positive = TRUE;

		if (y == piece->a.y || !piece->matrix[z][y - 1][x].value)
			cell->fields.faces.y_negative = TRUE;

		if (y == piece->b.y || !piece->matrix[z][y + 1][x].value)
			cell->fields.faces.y_positive = TRUE;

		if (z == piece->a.z || !piece->matrix[z - 1][y][x].value)
			cell->fields.faces.z_negative = TRUE;

		if (z == piece->b.z || !piece->matrix[z + 1][y][x].value)
			cell->fields.faces.z_positive = TRUE;
		}
	}


BLOCKOUT_API void blockout_piece_build(
	BlockoutPiece* piece,
	zuint8	       piece_set,
	zuint8	       piece_index
)
	{
	BlockoutCell *cell;
	zuint8 x, y, z, offset = 0;

	zuint32 polycube = polycubes
		[piece_set ? polycube_indices[piece_set - 1][piece_index] : piece_index];

	piece->a = z_3d_uint8
		( polycube >> 30,
		 (polycube >> 28) & 3,
		 (polycube >> 26) & 3);

	piece->b = z_3d_uint8
		(piece->a.x + ((polycube >> 23) & 7),
		 piece->a.y + ((polycube >> 20) & 7),
		 piece->a.z + ((polycube >> 17) & 7));

	z_block_int8_set(piece->matrix, 125 * sizeof(BlockoutCell), 0);

	for (z = piece->a.z; z <= piece->b.z; z++)
		for (y = piece->a.y; y <= piece->b.y; y++)
			for (x = piece->a.x; x <= piece->b.x; x++)
				if (polycube & (1 << offset++))
		{
		cell = &piece->matrix[z][y][x];
		cell->fields.piece_index = piece_index;
		cell->fields.faces.has = TRUE;
		}

	blockout_piece_update_faces(piece);
	}


static zuint8 bounds_hit(
	Blockout const*	     object,
	BlockoutPiece const* piece,
	Z3DSInt		     piece_point
)
	{
	return	piece_point.x + piece->a.x <  0			    ||
		piece_point.x + piece->b.x >= (zsint)object->size.x ||
		piece_point.y + piece->a.y <  0			    ||
		piece_point.y + piece->b.y >= (zsint)object->size.y ||
		piece_point.z + piece->a.z <  0
			? HIT_BOUNDS
			: (piece_point.z + piece->b.z >= (zsint)object->size.z
				? HIT_BOTTOM : 0);
	}


static zboolean content_hit(
	Blockout const*	     object,
	BlockoutPiece const* piece,
	Z3DSInt		     piece_point
)
	{
	zuint x, y, z, plane_size = object->size.x * object->size.y;

	for (z = piece->a.z; z <= piece->b.z; z++)
		for (y = piece->a.y; y <= piece->b.y; y++)
			for (x = piece->a.x; x <= piece->b.x; x++) if (
				piece->matrix[z][y][x].value &&
				object->matrix[
					(piece_point.z + z) * plane_size +
					(piece_point.y + y) * object->size.x +
					 piece_point.x + x
				].value
			)
				return TRUE;

	return FALSE;
	}


static void consolidate(Blockout *object)
	{
	zuint x, y, z;
	zuint plane_size = object->size.x * object->size.y;
	BlockoutPiece const *piece = object->piece;
	zuint16 cell;

	object->full_plane_count = 0;

	for (z = piece->a.z; z <= piece->b.z; z++)
		{
		for (y = piece->a.y; y <= piece->b.y; y++)
			for (x = piece->a.x; x <= piece->b.x; x++)
				if ((cell = piece->matrix[z][y][x].value))
					object->matrix[
						(object->piece_point.z + z) * plane_size +
						(object->piece_point.y + y) * object->size.x +
						 object->piece_point.x + x
					].value = cell;

		if (PLANE_IS_FULL
			(&object->matrix[plane_size * (object->piece_point.z + z)].value,
			 plane_size)
		)
			object->full_plane_indices[object->full_plane_count++]
			= object->piece_point.z + z;
		}

	if (object->piece_point.z + piece->a.z < (zsint)object->top)
		object->top = object->piece_point.z + piece->a.z;
	}


#define REVERSE(axis) point.axis = 4 - point.axis
#define SWAP(axis1, axis2) z_uint8_swap(&point.axis1, &point.axis2)


static Z3DUInt8 piece_point_rotate(Z3DUInt8 point, Z3DSInt8 rotation)
	{
	switch (rotation.x % 4)
		{
		case 0:				break;
		case 1: REVERSE(z); SWAP(z, y); break;
		case 2: REVERSE(y); REVERSE(z);	break;
		case 3: REVERSE(y); SWAP(z, y); break;
		}

	switch (rotation.y % 4)
		{
		case 0:				break;
		case 1: REVERSE(x); SWAP(x, z); break;
		case 2: REVERSE(z); REVERSE(x);	break;
		case 3: REVERSE(z); SWAP(x, z); break;
		}

	switch (rotation.z % 4)
		{
		case 0:				break;
		case 1: REVERSE(y); SWAP(y, x); break;
		case 2: REVERSE(x); REVERSE(y);	break;
		case 3: REVERSE(x); SWAP(y, x); break;
		}

	return point;
	}


BLOCKOUT_API void blockout_initialize(Blockout *object)
	{
	object->matrix = NULL;
	object->size   = z_3d_type_zero(UINT);
	}


BLOCKOUT_API void blockout_finalize(Blockout *object)
	{z_deallocate(object->matrix);}


BLOCKOUT_API ZStatus blockout_prepare(
	Blockout* object,
	Z3DUInt	  size,
	zuint8	  next_piece_set,
	zuint8	  next_piece_index
)
	{
	zusize matrix_size;

	if (	size.x < BLOCKOUT_MINIMUM_SIZE_X ||
		size.y < BLOCKOUT_MINIMUM_SIZE_Y ||
		size.z < BLOCKOUT_MINIMUM_SIZE_Z
	)
		return Z_ERROR_TOO_SMALL;

	if (	size.x > (zuint)Z_SINT_MAXIMUM ||
		size.y > (zuint)Z_SINT_MAXIMUM ||
		size.z > (zuint)Z_SINT_MAXIMUM ||
		z_type_multiplication_overflows_3(UINT)(size.x, size.y, size.z)
	)
		return Z_ERROR_TOO_BIG;

	if (	z_3d_type_inner_product(UINT)(object->size) !=
		(matrix_size = z_3d_type_inner_product(UINT)(size))
	)
		{
		void *matrix = z_reallocate
			(object->matrix, matrix_size * sizeof(BlockoutCell));

		if (matrix == NULL) return Z_ERROR_NOT_ENOUGH_MEMORY;
		object->matrix = matrix;
		}

	z_block_int8_set(object->matrix, matrix_size * sizeof(BlockoutCell), 0);

	object->size		 = size;
	object->top		 = size.z;
	object->piece		 = &object->pieces[0];
	object->next_piece	 = &object->pieces[1];
	object->full_plane_count = 0;
	object->next_piece_index = next_piece_index;

	blockout_piece_build(&object->pieces[1], next_piece_set, next_piece_index);
	return Z_OK;
	}


BLOCKOUT_API zboolean blockout_insert_piece(
	Blockout* object,
	zuint8	  next_piece_set,
	zuint8	  next_piece_index
)
	{
	object->piece_index	 = object->next_piece_index;
	object->next_piece_index = next_piece_index;
	z_type_swap(UINTPTR)(&object->piece, &object->next_piece);
	blockout_piece_build(object->next_piece, next_piece_set, next_piece_index);

	return !content_hit
		(object, object->piece,
		 object->piece_point = z_3d_type(SINT)
			((object->size.x - 5) / 2,
			 (object->size.y - 5) / 2,
			 -object->piece->a.z));
	}


BLOCKOUT_API BlockoutResult blockout_move_piece(Blockout *object, Z3DSInt8 movement)
	{
	BlockoutPiece *piece = object->piece;

	Z3DSInt point = z_3d_type(SINT)
		(object->piece_point.x + movement.x,
		 object->piece_point.y + movement.y,
		 object->piece_point.z + movement.z);

	zuint8 hit = bounds_hit(object, piece, point);

	if (hit)
		{
		if (hit == HIT_BOUNDS) return BLOCKOUT_RESULT_HIT;
		consolidate(object);
		return BLOCKOUT_RESULT_CONSOLIDATED;
		}

	if (content_hit(object, piece, point))
		{
		if (movement.z <= 0) return BLOCKOUT_RESULT_HIT;
		consolidate(object);
		return BLOCKOUT_RESULT_CONSOLIDATED;
		}

	object->piece_point = point;
	return Z_OK;
	}


BLOCKOUT_API BlockoutResult blockout_rotate_piece(Blockout *object, Z3DSInt8 rotation)
	{
	BlockoutPiece *piece = object->piece;
	BlockoutPiece rotated_piece;
	Z3DUInt8 a = piece_point_rotate(piece->a, rotation);
	Z3DUInt8 b = piece_point_rotate(piece->b, rotation);
	zuint x, y, z;
	zuint16 cell;
	Z3DUInt8 point;

	rotated_piece.b = z_3d_uint8_maximum(a, b);
	rotated_piece.a = z_3d_uint8_minimum(a, b);

	if (bounds_hit(object, &rotated_piece, object->piece_point))
		return BLOCKOUT_RESULT_HIT;

	z_block_int8_set(rotated_piece.matrix, 125 * sizeof(BlockoutCell), 0);

	for (z = piece->a.z; z <= piece->b.z; z++)
		for (y = piece->a.y; y <= piece->b.y; y++)
			for (x = piece->a.x; x <= piece->b.x; x++)
				if ((cell = piece->matrix[z][y][x].value))
		{
		point = piece_point_rotate(z_3d_uint8(x, y, z), rotation);

		rotated_piece.matrix[point.z][point.y][point.x].value
		= cell & (BLOCKOUT_CELL_MASK_SOLID | BLOCKOUT_CELL_MASK_PIECE_INDEX);
		}

	if (content_hit(object, &rotated_piece, object->piece_point))
		return BLOCKOUT_RESULT_HIT;

	z_copy(&rotated_piece, sizeof(BlockoutPiece), piece);
	blockout_piece_update_faces(piece);
	return Z_OK;
	}


BLOCKOUT_API void blockout_drop_piece(Blockout *object)
	{
	BlockoutPiece *piece = object->piece;
	Z3DSInt point = object->piece_point;

	for (	point.z++;
		!bounds_hit(object, piece, point) && !content_hit(object, piece, point);
		point.z++
	);

	object->piece_point.z = point.z - 1;
	consolidate(object);
	}


#define PLANES(index) (matrix + plane_size * (index))


BLOCKOUT_API void blockout_remove_full_planes(Blockout *object)
	{
	zuint8 plane_count = object->full_plane_count;

	if (plane_count)
		{
		zuint *full_plane_indices = object->full_plane_indices;
		zuint delta = 0, size, plane_size = object->size.x * object->size.y;
		zuint8 index;
		BlockoutCell *matrix = object->matrix, *plane, *plane_above, *plane_below, cell;

		while (plane_count)
			{
			plane = PLANES(full_plane_indices[--plane_count]);
			plane_above = plane - plane_size;
			plane_below = plane + plane_size;

			for (index = plane_size; index;)
				{
				if (!(cell = plane[--index]).fields.faces.z_negative)
					plane_above[index].fields.faces.z_positive = TRUE;

				if (!cell.fields.faces.z_positive)
					plane_below[index].fields.faces.z_negative = TRUE;
				}
			}

		for (index = plane_count = object->full_plane_count; --index;)
			{
			delta++;

			if ((size = full_plane_indices[index] - full_plane_indices[index - 1] - 1))
				z_move	(PLANES(full_plane_indices[index - 1] + 1),
					 plane_size * size * sizeof(BlockoutCell),
					 PLANES(full_plane_indices[index - 1] + 1 + delta));
			}

		z_move	(PLANES(object->top),
			 plane_size * (full_plane_indices[0] - object->top) * sizeof(BlockoutCell),
			 PLANES(object->top + plane_count));

		z_block_int8_set
			(PLANES(object->top),
			 plane_size * plane_count * sizeof(BlockoutCell), 0);

		object->full_plane_count = 0;
		object->top += plane_count;
		}
	}


/* Blockout.c EOF */
