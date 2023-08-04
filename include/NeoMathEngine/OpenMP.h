/* Copyright © 2017-2023 ABBYY

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
--------------------------------------------------------------------------------------------------------------*/

#pragma once

#ifdef NEOML_USE_OMP
#include <omp.h>
#endif

#include <cstdint>

#ifdef NEOML_USE_OMP
#if defined( _MSC_VER )
#define NEOML_OMP(cond, x) __pragma(omp x if(cond))
#define NEOML_OMP_BARRIER(cond) do { if(cond) { __pragma(omp barrier) } } while(0)
#define NEOML_OMP_NUM_THREADS( threadCount ) __pragma(omp parallel num_threads( threadCount ) if( threadCount > 1 ) )
#define NEOML_OMP_FOR_NUM_THREADS( threadCount ) __pragma(omp parallel for num_threads( threadCount ) if( threadCount > 1 ) )
#else
#define STRINGIFY(a) #a
#define NEOML_OMP(cond, x) _Pragma("omp x if(cond)")
#define NEOML_OMP_BARRIER(cond) do { if(cond) { _Pragma("omp barrier") } } while(0)
#define NEOML_OMP_NUM_THREADS( threadCount ) _Pragma( STRINGIFY( omp parallel num_threads( threadCount ) if( threadCount > 1 ) ) )
#define NEOML_OMP_FOR_NUM_THREADS( threadCount ) _Pragma( STRINGIFY( omp parallel for num_threads( threadCount ) if( threadCount > 1 ) ) )
#endif
#else  // !NEOML_USE_OMP
#define NEOML_OMP(cond, x)  { const bool __attribute__((unused)) tempCond = cond; }
#define NEOML_OMP_BARRIER(cond)  { const bool __attribute__((unused)) tempCond = cond; }
#define NEOML_OMP_NUM_THREADS( threadCount ) { const int __attribute__((unused)) tempThreadCount = threadCount; }
#define NEOML_OMP_FOR_NUM_THREADS( threadCount ) { const int __attribute__((unused)) tempThreadCount = threadCount; }
#endif // !NEOML_USE_OMP

namespace NeoML {

#define NEOML_OMP_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define NEOML_OMP_MIN(x, y) (((x) < (y)) ? (x) : (y))

const int MinOmpOperationCount = 32768;

// Indicates if using the OMP pool makes sense for the scenario
// taskCount is the number of tasks that may run on a thread
// operationCount is the total number of operations across all tasks
inline bool IsOmpRelevant( int taskCount, int64_t operationCount = MinOmpOperationCount )
{
#ifdef NEOML_USE_OMP
	return taskCount > 1 && operationCount >= MinOmpOperationCount;
#else  // !NEOML_USE_OMP
	(void)MinOmpOperationCount;
	(void)taskCount;
	(void)operationCount;
	return false;
#endif // !NEOML_USE_OMP
}

// Returns the maximum possible number of threads used in an OMP block
inline int OmpGetMaxThreadCount()
{
#ifdef NEOML_USE_OMP
	return omp_get_max_threads();
#else  // !NEOML_USE_OMP
	return 1;
#endif // !NEOML_USE_OMP
}

// Returns the current number of threads in the OMP pool
inline int OmpGetThreadCount()
{
#ifdef NEOML_USE_OMP
	return omp_get_num_threads();
#else  // !NEOML_USE_OMP
	return 1;
#endif // !NEOML_USE_OMP
}

// Returns the current thread number
inline int OmpGetThreadNum()
{
#ifdef NEOML_USE_OMP
	return omp_get_thread_num();
#else  // !NEOML_USE_OMP
	return 0;
#endif // !NEOML_USE_OMP
}

// Splits the specified number of tasks [0, fullCount) into intervals for each thread
// Returns the index of the first task and the number of tasks per thread
// IMPORTANT: may return 0 as the number of tasks
// The return value is true if count != 0 (there are some tasks), otherwise false
inline bool OmpGetTaskIndexAndCount( int fullCount, int align, int& index, int& count,
	int threadCount = OmpGetThreadCount(), int threadNum = OmpGetThreadNum() )
{
	if( threadCount > 1 ) {
		int countPerThread = ( fullCount + threadCount - 1 ) / threadCount;
		countPerThread = ( align > 1 ) ? ( ( countPerThread + align - 1 ) / align * align ) : countPerThread;

		index = countPerThread * threadNum;
		count = countPerThread;
		count = NEOML_OMP_MIN( count, fullCount - index );
		count = NEOML_OMP_MAX( count, 0 );
	} else {
		index = 0;
		count = fullCount;
	}

	return count != 0;
}

inline bool OmpGetTaskIndexAndCount( int fullCount, int& index, int& count,
	int ompThreadCount = OmpGetThreadCount(), int ompThreadNum = OmpGetThreadNum() )
{
	return OmpGetTaskIndexAndCount( fullCount, /*align*/1, index, count, ompThreadCount, ompThreadNum );
}

inline int GreatestCommonFactorWithAlign( int m, int mAlign, int n )
{
	if( ( m % mAlign ) == 0 ) {
		// If "m" was aligned to start with, preserve the alignment
		m /= mAlign;
	}

	// Euclidean algorithm
	while( true ) {
		int k = m % n;
		if( k == 0 ) {
			return n;
		}
		m = n;
		n = k;
	}
}

// Similar to OmpGetTaskIndexAndCount, only for a 3D "cube" of tasks to be split among OMP threads
inline bool OmpGetTaskIndexAndCount3D( int fullCountX, int alignX, int fullCountY, int alignY, int fullCountZ, int alignZ,
	int& indexX, int& countX, int& indexY, int& countY, int& indexZ, int& countZ,
	int ompThreadCount = OmpGetThreadCount(), int ompThreadNum = OmpGetThreadNum() )
{
	if( ompThreadCount == 1 ) {
		indexX = 0;
		countX = fullCountX;
		indexY = 0;
		countY = fullCountY;
		indexZ = 0;
		countZ = fullCountZ;
	} else {
		int threadCount = ompThreadCount;
		// Calculate the thread block size: = mulX x mulY x mulZ
		// Attempt to divide without a remainder if possible
		int mulX = GreatestCommonFactorWithAlign( fullCountX, alignX, threadCount );
		threadCount /= mulX;
		int mulY = GreatestCommonFactorWithAlign( fullCountY, alignY, threadCount );
		threadCount /= mulY;
		int mulZ = GreatestCommonFactorWithAlign( fullCountZ, alignZ, threadCount );
		threadCount /= mulZ;

		countX = fullCountX / mulX;
		countY = fullCountY / mulY;
		countZ = fullCountZ / mulZ;

		// Find the maximum dimension and divide by it, with remainder if necessary
		int* maxMul = &mulX;
		int* maxCount = &countX;
		const int* align = &alignX;

		if( countY / alignY > *maxCount / *align ) {
			maxMul = &mulY;
			maxCount = &countY;
			align = &alignY;
		}
		if( countZ / alignZ > *maxCount / *align ) {
			maxMul = &mulZ;
			maxCount = &countZ;
		}

		*maxCount = ( *maxCount + threadCount - 1 ) / threadCount;
		*maxMul *= threadCount;

		// Align the block size
		countX = ( countX + alignX - 1 ) / alignX * alignX;
		countY = ( countY + alignY - 1 ) / alignY * alignY;
		countZ = ( countZ + alignZ - 1 ) / alignZ * alignZ;

		// Calculate the coordinate for the given OMP thread in a block
		int threadNumX = ompThreadNum % mulX;
		int threadNumY = ompThreadNum / mulX;
		int threadNumZ = threadNumY / mulY;
		threadNumY %= mulY;

		// Calculate indeces
		indexX = countX * threadNumX;
		countX = NEOML_OMP_MIN( countX, fullCountX - indexX );
		countX = NEOML_OMP_MAX( countX, 0 );

		indexY = countY * threadNumY;
		countY = NEOML_OMP_MIN( countY, fullCountY - indexY );
		countY = NEOML_OMP_MAX( countY, 0 );

		indexZ = countZ * threadNumZ;
		countZ = NEOML_OMP_MIN( countZ, fullCountZ - indexZ );
		countZ = NEOML_OMP_MAX( countZ, 0 );
	}
	return countX != 0 && countY != 0 && countZ != 0;
}

inline bool OmpGetTaskIndexAndCount3D( int fullCountX, int fullCountY, int fullCountZ,
	int& indexX, int& countX, int& indexY, int& countY, int& indexZ, int& countZ,
	int ompThreadCount = OmpGetThreadCount(), int ompThreadNum = OmpGetThreadNum() )
{
	return OmpGetTaskIndexAndCount3D( fullCountX, 1, fullCountY, 1, fullCountZ, 1,
		indexX, countX, indexY, countY, indexZ, countZ,
		ompThreadCount, ompThreadNum );
}

inline bool OmpGetTaskIndexAndCount2D( int fullCountX, int alignX, int fullCountY, int alignY,
	int& indexX, int& countX, int& indexY, int& countY,
	int ompThreadCount = OmpGetThreadCount(), int ompThreadNum = OmpGetThreadNum() )
{
	int indexZ = 0;
	int countZ = 0;
	return OmpGetTaskIndexAndCount3D( fullCountX, alignX, fullCountY, alignY, 1, 1,
		indexX, countX, indexY, countY, indexZ, countZ,
		ompThreadCount, ompThreadNum );
}

inline bool OmpGetTaskIndexAndCount2D( int fullCountX, int fullCountY,
	int& indexX, int& countX, int& indexY, int& countY,
	int ompThreadCount = OmpGetThreadCount(), int ompThreadNum = OmpGetThreadNum() )
{
	return OmpGetTaskIndexAndCount2D( fullCountX, 1, fullCountY, 1,
		indexX, countX, indexY, countY,
		ompThreadCount, ompThreadNum );
}

} // namespace NeoML
