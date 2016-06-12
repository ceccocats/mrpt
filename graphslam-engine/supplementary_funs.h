/* +---------------------------------------------------------------------------+
   |                     Mobile Robot Programming Toolkit (MRPT)               |
   |                          http://www.mrpt.org/                             |
   |                                                                           |
   | Copyright (c) 2005-2016, Individual contributors, see AUTHORS file        |
   | See: http://www.mrpt.org/Authors - All rights reserved.                   |
   | Released under BSD License. See details in http://www.mrpt.org/License    |
   +---------------------------------------------------------------------------+ */

/**
 * List of, supplementary to the problem functions. 
 * TODO: Consider integrating them into their respective
 * namespaces/implementation files..
 */

#ifndef SUPPLEMENTARY_FUNS_H
#define SUPPLEMENTARY_FUNS_H


#include <mrpt/graphslam.h>

namespace supplementary_funs {

	/** 
	 * levMarqFeedback
	 *
	 * Feedback fucntion for the graph optimization
	 */
	template <
		class GRAPH_T,
		class NODE_REGISTRATOR, 
		class EDGE_REGISTRATOR>
			void levMarqFeedback(
					const GRAPH_T &graph,
					const size_t iter,
					const size_t max_iter,
					const double cur_sq_error )
			{
			}

}


#endif /* end of include guard: SUPPLEMENTARY_FUNS_H */
