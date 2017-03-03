// K-Dimensional Tree class interface
// Copied from http://codereview.stackexchange.com/questions/110225/k-d-tree-implementation-in-c11
#pragma once

#include "types.h"
#include <unordered_set>

namespace DirectGraphicalModels
{
	template <unsigned int K> class kd_tree_iterator;

	template <unsigned int K>
	class CKDTree
	{
	public:
		using kd_Box	= std::pair<vec_float_t, vec_float_t>;

		class vec_float_t_Hasher
		{
		public:
			size_t operator() (const vec_float_t &point) const
			{
				size_t retVal = 0;
				for (float p : point) 
					retVal ^= hash<float>()(p);				// !!! (1,2) and (2,1) hash to the same value
					//boost::hash_combine(retVal, p);
				return retVal;
			}
		};


	protected:
		// Node class - internal to KD tree
		class kd_node
		{
		public:
			virtual bool		isInternal(void) const = 0;
			virtual void		SearchKdTree(const kd_Box &searchBox, std::vector<vec_float_t> &Points, const unsigned Depth = 0) const = 0;
			virtual void		FindNearestNeighbor(const vec_float_t &srcPoint, vec_float_t &nearPoint, float &minDistance, kd_Box &minRegion, const unsigned int Depth = 0) const = 0;
			virtual void		FindKNearestNeighbors(const vec_float_t &srcPoint, std::vector<vec_float_t> &nearPoints, const unsigned k, float &minDistance, kd_Box &minRegion, std::unordered_set<vec_float_t, vec_float_t_Hasher> &nearSet, const unsigned int Depth = 0) const = 0;
			virtual unsigned	TreeHeight(void) const = 0;
			virtual unsigned	nodeCount(bool withInternalNodes) const = 0;
			virtual kd_Box		boundingBox(void) const = 0;
		};

		// Internal node class
		class kd_internal_node : public kd_node
		{
		private:
			float m_splitVal;
			kd_Box m_boundingBox;
			std::shared_ptr<kd_node> m_Left, m_Right;

		public:
			kd_internal_node(const float splitVal, kd_Box &boundingBox, std::shared_ptr<kd_node> Left) :
				m_splitVal(splitVal), m_boundingBox(boundingBox), m_Left(Left) {}

			kd_internal_node(const float splitVal, kd_Box &boundingBox, std::shared_ptr<kd_node> Left, std::shared_ptr<kd_node> Right) :
				m_splitVal(splitVal), m_boundingBox(boundingBox), m_Left(Left), m_Right(Right) {}

			float splitVal() const { return m_splitVal; }

			std::shared_ptr<kd_node> Left() const { return m_Left; }
			std::shared_ptr<kd_node> Right() const { return m_Right; }

			virtual bool isInternal() const override { return true; }

			virtual kd_Box boundingBox() const override { return m_boundingBox; }

			virtual void SearchKdTree(const kd_Box &searchBox, std::vector<vec_float_t> &Points, const unsigned Depth) const override
			{
				if (regionCrossesRegion(searchBox, m_Left->boundingBox()))
					m_Left->SearchKdTree(searchBox, Points, Depth + 1);

				if (m_Right != nullptr && regionCrossesRegion(searchBox, m_Right->boundingBox()))
					m_Right->SearchKdTree(searchBox, Points, Depth + 1);
			}

			virtual void FindKNearestNeighbors(const vec_float_t &srcPoint, std::vector<vec_float_t> &nearPoints, const unsigned k,
				float &minDistance, kd_Box &minRegion, std::unordered_set<vec_float_t, vec_float_t_Hasher> &nearSet,
				const unsigned int Depth = 0) const override
			{
				if (regionCrossesRegion(m_Left->boundingBox(), minRegion))
					m_Left->FindKNearestNeighbors(srcPoint, nearPoints, k, minDistance, minRegion, nearSet, Depth + 1);

				if (m_Right != nullptr && regionCrossesRegion(m_Right->boundingBox(), minRegion))
					m_Right->FindKNearestNeighbors(srcPoint, nearPoints, k, minDistance, minRegion, nearSet, Depth + 1);
			}

			virtual void FindNearestNeighbor(const vec_float_t &srcPoint, vec_float_t &nearPoint,
				float &minDistance, kd_Box &minRegion, const unsigned int Depth = 0) const override
			{
				if (regionCrossesRegion(m_Left->boundingBox(), minRegion))
					m_Left->FindNearestNeighbor(srcPoint, nearPoint, minDistance, minRegion, Depth + 1);

				if (m_Right != nullptr && regionCrossesRegion(m_Right->boundingBox(), minRegion))
					m_Right->FindNearestNeighbor(srcPoint, nearPoint, minDistance, minRegion, Depth + 1);
			}

			virtual unsigned TreeHeight() const override
			{
				return 1 + max(m_Left->TreeHeight(),
					m_Right != nullptr ? m_Right->TreeHeight() : 0);
			}

			virtual unsigned nodeCount(bool withInternalNodes) const override
			{
				return withInternalNodes ? 1 : 0 +
					m_Left->nodeCount(withInternalNodes) +
					(m_Right != nullptr ? m_Right->nodeCount(withInternalNodes) : 0);
			}
		};

		// Lead node
		class kd_leaf_node : public kd_node
		{
		private:
			vec_float_t m_pointCoords;

		public:
			std::weak_ptr<kd_leaf_node> m_Next, m_Prev;

			kd_leaf_node(const vec_float_t &Point) : m_pointCoords(Point) { }

			vec_float_t pointCoords() const { return m_pointCoords; }

			virtual bool isInternal() const override { return false; }

			virtual unsigned TreeHeight() const override { return 1; }

			virtual unsigned nodeCount(bool withInternalNodes) const override { return 1; }

			virtual void SearchKdTree(const kd_Box &searchBox, std::vector<vec_float_t> &Points, const unsigned Depth) const override
			{
				if (pointIsInRegion(m_pointCoords, searchBox))
					Points.push_back(m_pointCoords);
			}

			virtual kd_Box boundingBox() const override
			{
				kd_Box bounding_box;

				bounding_box.first = bounding_box.second = m_pointCoords;

				return bounding_box;
			}

			virtual void FindNearestNeighbor(const vec_float_t &srcPoint, vec_float_t &nearPoint,
				float &minDistance, kd_Box &minRegion, const unsigned int Depth = 0) const override
			{
				if (Distance(srcPoint, m_pointCoords) <= minDistance)
				{
					nearPoint = m_pointCoords;
					minDistance = Distance(srcPoint, nearPoint);

					for (unsigned index = 0; index < K; index++)
					{
						minRegion.first[index] = srcPoint[index] - minDistance;
						minRegion.second[index] = srcPoint[index] + minDistance;
					}
				}
			}

			virtual void FindKNearestNeighbors(const vec_float_t &srcPoint, std::vector<vec_float_t> &nearPoints, const unsigned k,
				float &minDistance, kd_Box &minRegion, std::unordered_set<vec_float_t, vec_float_t_Hasher> &nearSet,
				const unsigned int Depth = 0) const override
			{
				if (Distance(srcPoint, m_pointCoords) <= minDistance && nearSet.find(m_pointCoords) == nearSet.end())
				{
					nearSet.erase(nearPoints[k - 1]);
					nearSet.insert(m_pointCoords);

					nearPoints[k - 1] = m_pointCoords;

					for (unsigned i = k - 1; i > 0; i--)
						if (Distance(srcPoint, nearPoints[i - 1]) > Distance(srcPoint, nearPoints[i]))
							swap(nearPoints[i - 1], nearPoints[i]);
						else
							break;

					minDistance = Distance(srcPoint, nearPoints[k - 1]);

					for (unsigned index = 0; index < K; index++)
					{
						minRegion.first[index] = srcPoint[index] - minDistance;
						minRegion.second[index] = srcPoint[index] + minDistance;
					}
				}
			}
		};

		// --------------------------------
		std::shared_ptr<kd_node>		m_Root;
		std::weak_ptr<kd_leaf_node>		m_firstLeaf;
		// -----------------------------------------------------------------------------------------------------

		// The routine has a desired side effect of sorting Points
		float							NthCoordMedian(std::vector<vec_float_t> &Points, const unsigned num)
		{
			sort(Points.begin(), Points.end(), [num](vec_float_t &A, vec_float_t &B) { return A[num%K] < B[num%K]; });

			float Median = Points[Points.size() / 2][num];

			if (Points.size() % 2 == 0)
				Median = (Median + Points[Points.size() / 2 - 1][num]) / 2.0f;

			if (Median == Points[Points.size() - 1][num] &&
				Median != Points[0][num])
			{
				int index = Points.size() / 2;

				while (Median == Points[--index][num]);

				Median = (Median + Points[index][num]) / 2.0f;
			}

			return Median;
		}
		std::shared_ptr<kd_node>		CreateTree(std::vector<vec_float_t> &Points, std::shared_ptr<kd_leaf_node> &Last_Leaf, const unsigned int Depth = 0)
		{
			if (Points.size() == 1)
			{
				shared_ptr<kd_leaf_node> retNode(new kd_leaf_node(Points[0]));

				if (Last_Leaf)
				{
					Last_Leaf->m_Next = retNode;
					retNode->m_Prev = Last_Leaf;
				}
				else
					m_firstLeaf = retNode;

				Last_Leaf = retNode;

				return retNode;
			}
			else if (Points.size() == 2)
			{
				if (Points[0][Depth%K] == Points[1][Depth%K])
				{
					shared_ptr<kd_node> Left = CreateTree(Points, Last_Leaf, Depth + 1);

					shared_ptr<kd_internal_node> retNode(new kd_internal_node(Points[0][Depth%K], Left->boundingBox(), Left));

					return retNode;
				}
				else
				{
					if (Points[0][Depth%K] > Points[1][Depth%K])
						swap(Points[0], Points[1]);

					shared_ptr<kd_leaf_node> Left(new kd_leaf_node(Points[0])), Right(new kd_leaf_node(Points[1]));

					if (Last_Leaf)
					{
						Last_Leaf->m_Next = Left;
						Left->m_Prev = Last_Leaf;
					}
					else
						m_firstLeaf = Left;

					Left->m_Next = Right;
					Right->m_Prev = Left;

					Last_Leaf = Right;

					kd_Box boundingBox;

					for (unsigned i = 0; i < K; i++)
					{
						boundingBox.first[i] = min(Points[0][i], Points[1][i]);
						boundingBox.second[i] = max(Points[0][i], Points[1][i]);
					}

					shared_ptr<kd_internal_node> retNode(new
						kd_internal_node((Points[0][Depth%K] + Points[1][Depth%K]) / 2.0f, boundingBox, Left, Right));

					return retNode;
				}
			}
			else
			{
				float Median = NthCoordMedian(Points, Depth%K);

				vector<vec_float_t> subtreePoints;
				shared_ptr<kd_node> Left, Right;

				subtreePoints.reserve(Points.size() / 2);

				for (size_t index = 0; index < Points.size() && Points[index][Depth%K] <= Median; index++)
					subtreePoints.push_back(Points[index]);

				if (subtreePoints.size() > 0)
					Left = CreateTree(subtreePoints, Last_Leaf, Depth + 1);

				unsigned int insertedPoints = subtreePoints.size();
				unsigned int remainedPoints = Points.size() - subtreePoints.size();

				subtreePoints.resize(remainedPoints); subtreePoints.shrink_to_fit();

				for (size_t index = insertedPoints; index < Points.size(); index++)
					subtreePoints[index - insertedPoints] = Points[index];

				if (subtreePoints.size() > 0)
					Right = CreateTree(subtreePoints, Last_Leaf, Depth + 1);

				subtreePoints.resize(0); subtreePoints.shrink_to_fit();

				kd_Box boundingBox;

				if (Right)
				{
					kd_Box lbb = Left->boundingBox(),
						rbb = Right->boundingBox();

					for (unsigned i = 0; i < K; i++)
					{
						boundingBox.first[i] = min(lbb.first[i], rbb.first[i]);
						boundingBox.second[i] = max(lbb.second[i], rbb.second[i]);
					}
				}
				else
					boundingBox = Left->boundingBox();

				shared_ptr<kd_internal_node> retNode(new kd_internal_node(Median, boundingBox, Left, Right));

				return retNode;
			}
		}
		std::shared_ptr<kd_leaf_node>	ApproxNearestNeighborNode(const vec_float_t &srcPoint) const
		{
			unsigned int Depth = 0;
			shared_ptr<kd_node> Node(m_Root);

			while (Node->isInternal())
			{
				shared_ptr<kd_internal_node> iNode = static_pointer_cast<kd_internal_node>(Node);

				if (srcPoint[Depth++%K] <= iNode->splitVal() || iNode->Right() == nullptr)
					Node = iNode->Left();
				else
					Node = iNode->Right();
			}

			shared_ptr<kd_leaf_node> lNode = static_pointer_cast<kd_leaf_node>(Node);

			return lNode;
		}
		float							ApproxNearestNeighborDistance(const vec_float_t &srcPoint) const
		{
			shared_ptr<kd_leaf_node> node = ApproxNearestNeighborNode(srcPoint);

			return Distance(srcPoint, node->pointCoords());
		}
		void							PrintTree(std::shared_ptr<kd_node> node, unsigned int depth = 0) const
		{
			for (unsigned i = 0; i < depth; i++)
				cout << " ";

			if (node == nullptr)
				cout << "null" << endl;
			else
			{
				if (node->isInternal())
				{
					shared_ptr<kd_internal_node> iNode = static_pointer_cast<kd_internal_node>(node);

					cout << "Split val is " << iNode->splitVal() << " for axis #" << depth%K + 1 << endl;

					PrintTree(iNode->m_Left, depth + 1);
					PrintTree(iNode->m_Right, depth + 1);
				}
				else
				{
					shared_ptr<kd_leaf_node> lNode = static_pointer_cast<kd_leaf_node>(node);

					cout << "Point is (";

					for (unsigned i = 0; i < K; i++)
						cout << lNode->m_Vals[i] << " ";

					cout << ")" << endl;
				}
			}
		}


	public:
		CKDTree(void) { }
		CKDTree(std::vector<vec_float_t> &Points) { insert(Points); }
		CKDTree(const CKDTree &obj) = delete;

		bool operator==(const CKDTree<K> rhs) = delete;
		bool operator=(const CKDTree<K> rhs) = delete;

		void clear(void) { m_Root.reset(); m_firstLeaf.reset(); }

		friend class kd_tree_iterator<K>;
		typedef typename kd_tree_iterator<K> iterator;

		kd_tree_iterator<K> end();
		kd_tree_iterator<K> begin();

		static bool		pointIsInRegion(const vec_float_t &Point, const std::pair<vec_float_t, vec_float_t> &Region);
		static bool		regionCrossesRegion(const std::pair<vec_float_t, vec_float_t> &Region1, const std::pair<vec_float_t, vec_float_t> &Region2);
		static float	Distance(const vec_float_t &P, const vec_float_t &Q);

		void			insert(std::vector<vec_float_t> &Points)
		{
			clear();

			for (signed i = Points.size() - 1; i >= 0; i--)
				if (!Points[i].isValid())
					Points.erase(Points.begin() + i);

			if (Points.size() > 0)
			{
				sort(Points.begin(), Points.end());
				vector<vec_float_t>::iterator it = unique(Points.begin(), Points.end());
				Points.resize(distance(Points.begin(), it));

				shared_ptr<kd_leaf_node> dummyLeaf;

				m_Root = CreateTree(Points, dummyLeaf);
			}
		}
		void			search(const vec_float_t &minPoint, const vec_float_t &maxPoint, std::vector<vec_float_t> &Points) const
		{
			Points.clear();

			if (m_Root == nullptr)
				return;

			kd_Box sorted;

			for (unsigned coord = 0; coord < K; coord++)
			{
				sorted.first[coord] = min(minPoint[coord], maxPoint[coord]);
				sorted.second[coord] = max(minPoint[coord], maxPoint[coord]);
			}

			m_Root->SearchKdTree(sorted, Points);
		}
		bool			FindNearestNeighbor(const vec_float_t &srcPoint, vec_float_t &nearPoint) const
		{
			bool retVal = (m_Root != nullptr);

			if (!m_Root)
				nearPoint.fill(numeric_limits<float>::quiet_NaN());
			else
			{
				float minDistance = ApproxNearestNeighborDistance(srcPoint);

				kd_Box minBox;

				for (unsigned coord = 0; coord < K; coord++)
				{
					minBox.first[coord] = srcPoint[coord] - minDistance;
					minBox.second[coord] = srcPoint[coord] + minDistance;
				}

				m_Root->FindNearestNeighbor(srcPoint, nearPoint, minDistance, minBox);
			}

			return retVal;
		}
		bool			FindKNearestNeighbors(const vec_float_t &srcPoint, std::vector<vec_float_t> &nearPoints, const unsigned k) const
		{
			nearPoints.clear();
			nearPoints.reserve(k);

			if (!m_Root) return false;

			shared_ptr<kd_leaf_node> nNode = ApproxNearestNeighborNode(srcPoint),
				pNode = nNode->m_Prev.lock();

			nearPoints.push_back(nNode->pointCoords());

			nNode = nNode->m_Next.lock();

			while (nearPoints.size() < k && (nNode || pNode))
			{
				if (nNode)
				{
					nearPoints.push_back(nNode->pointCoords());

					nNode = nNode->m_Next.lock();
				}

				if (pNode && nearPoints.size() < k)
				{
					nearPoints.push_back(pNode->pointCoords());

					pNode = pNode->m_Prev.lock();
				}
			}

			sort(nearPoints.begin(), nearPoints.end(),
				[srcPoint](vec_float_t &A, vec_float_t &B) {return Distance(srcPoint, A) < Distance(srcPoint, B); });

			float minDistance;

			if (nearPoints.size() < k)
			{
				vec_float_t infinityPoint;
				infinityPoint.fill(numeric_limits<float>::infinity());

				nearPoints.resize(k, infinityPoint);

				minDistance = numeric_limits<float>::infinity();
			}
			else
				minDistance = Distance(srcPoint, nearPoints[k - 1]);

			kd_Box MinBox;

			for (unsigned i = 0; i < K; i++)
			{
				MinBox.first[i] = srcPoint[i] - minDistance;
				MinBox.second[i] = srcPoint[i] + minDistance;
			}

			unordered_set<vec_float_t, vec_float_t_Hasher> nearSet(nearPoints.begin(), nearPoints.end());

			m_Root->FindKNearestNeighbors(srcPoint, nearPoints, k, minDistance, MinBox, nearSet);

			for (signed i = k - 1; i > 0 && !nearPoints[i].isValid(); i--)
				nearPoints.erase(nearPoints.begin() + i);

			return true;
		}
		unsigned		nodeCount(bool withInternalNodes = false) const
		{
			return m_Root != nullptr ? m_Root->nodeCount(withInternalNodes) : 0;
		}
		unsigned		TreeHeight(void) const
		{
			return this->m_Root != nullptr ? m_Root->TreeHeight() : 0;
		}
		void			PrintTree(void) const { PrintTree(m_Root); }
	};

	// -------------------------------------------------------------

	template <unsigned int K>
	float CKDTree<K>::Distance(const vec_float_t &P, const vec_float_t &Q)
	{
		float Sum = 0;

		for (unsigned i = 0; i < K; i++)
			Sum += (P[i] - Q[i]) * (P[i] - Q[i]);

		return sqrtf(Sum);
	}

	template <unsigned int K>
	bool CKDTree<K>::pointIsInRegion(const vec_float_t &point, const std::pair<vec_float_t, vec_float_t> &Region)
	{
		for (size_t i = 0; i < point.size(); i++)
			if (!(Region.first[i] <= point[i] && point[i] <= Region.second[i]))
				return false;

		return true;
	}

	template <unsigned int K>
	bool CKDTree<K>::regionCrossesRegion(const std::pair<vec_float_t, vec_float_t> &Region1, const std::pair<vec_float_t, vec_float_t> &Region2)
	{
		bool regionsCross = true;

		for (unsigned i = 0; i < K && regionsCross; i++)
			if (Region1.first[i] > Region2.second[i] || Region1.second[i] < Region2.first[i])
				regionsCross = false;

		return regionsCross;
	}

	// -----------------------
	//
	// Iterator implementation
	//
	template <unsigned int K>
	class kd_tree_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void>
	{
	public:
		kd_tree_iterator() {}
		kd_tree_iterator(std::shared_ptr<typename CKDTree<K>::kd_leaf_node> node) : nodePtr(node) {}

		template <unsigned int K> friend bool operator== (const kd_tree_iterator<K>& lhs, const kd_tree_iterator<K>& rhs);
		template <unsigned int K> friend bool operator!= (const kd_tree_iterator<K>& lhs, const kd_tree_iterator<K>& rhs);

		typename CKDTree<K>::vec_float_t& operator*()
		{
			return this->nodePtr->pointCoords();
		}

		kd_tree_iterator& operator++()
		{
			this->nodePtr = this->nodePtr->m_Next.lock();

			return *this;
		}

		kd_tree_iterator operator++(int)
		{
			kd_tree_iterator tmp(*this);
			operator++();
			return tmp;
		}

	private:
		std::shared_ptr<typename CKDTree<K>::kd_leaf_node> nodePtr;
	};

	template <unsigned int K>
	bool operator== (const kd_tree_iterator<K>& lhs, const kd_tree_iterator<K>& rhs)
	{
		return (lhs.nodePtr == rhs.nodePtr);
	}

	template <unsigned int K>
	bool operator!= (const kd_tree_iterator<K> &lhs, const kd_tree_iterator<K>& rhs)
	{
		return !(lhs == rhs);
	}

	template <unsigned int K>
	kd_tree_iterator<K> CKDTree<K>::end()
	{
		kd_tree<K>::iterator retVal;

		return retVal;
	}

	template <unsigned int K>
	kd_tree_iterator<K> CKDTree<K>::begin()
	{
		if (!this->m_Root)
			return end();
		else
			return m_firstLeaf.lock();
	}
}