import json
from dataclasses import dataclass, replace
from collections import defaultdict

# Reproduce the voting system

@dataclass
class Transform:
	flipX: bool
	flipY: bool
	rotation: int

	def __hash__(self):
		return hash(self.rotation + 4 * (self.flipX + 2 * self.flipY))

	def isIdentity(self):
		return not self.flipX and not self.flipY and self.rotation == 0

@dataclass
class Score:
	total_votes: int = 0
	untransformed_votes: int = 0
	flipped_edges: int = 0

	def __gt__(self, other):
		if self.total_votes > other.total_votes:
			return True
		if self.total_votes < other.total_votes:
			return False
		if self.flipped_edges < other.flipped_edges:
			return True
		if self.flipped_edges > other.flipped_edges:
			return False
		if self.untransformed_votes > other.untransformed_votes:
			return True
		if self.untransformed_votes < other.untransformed_votes:
			return False
		return False

def computeAllTransforms():
	all_transforms = [
		Transform(False, False, 0)
	]
	all_transforms = [
		x
		for tr in all_transforms
		for x in [replace(tr), replace(tr, flipX=True)]
	]
	all_transforms = [
		x
		for tr in all_transforms
		for x in [replace(tr), replace(tr, flipY=True)]
	]
	all_transforms = [
		replace(tr, rotation=r)
		for tr in all_transforms
		for r in range(4)
	]
	return all_transforms

def computeAllTransforms_check():
	all_transforms = []
	for fx in [False, True]:
		for fy in [False, True]:
			for rot in range(4):
				all_transforms.append(Transform(fx, fy, rot))
	return all_transforms

all_transforms = computeAllTransforms()
assert(len(all_transforms) == 16)

check_all_transforms = computeAllTransforms_check()
assert(check_all_transforms == all_transforms)

def hash_tile(tile):
	return "".join(map(str, tile))

def transform_tile(tile, transform):
	def flipX(tile):
		return [
			+tile[2],
			-tile[1],
			+tile[0],
			-tile[3],
		]
	def flipY(tile):
		return [
			-tile[0],
			+tile[3],
			-tile[2],
			+tile[1],
		]
	def rotate(tile, r):
		return [
			tile[(0 + r) % 4],
			tile[(1 + r) % 4],
			tile[(2 + r) % 4],
			tile[(3 + r) % 4],
		]
	if transform.flipX:
		tile = flipX(tile)
	if transform.flipY:
		tile = flipY(tile)
	tile = rotate(tile, transform.rotation)
	return tile

def consolidateVotes(neighborhood_labels):
	possibilities = []
	for neighborhood in neighborhood_labels:
		for label0 in neighborhood[0]:
			for label1 in neighborhood[1]:
				for label2 in neighborhood[2]:
					for label3 in neighborhood[3]:
						possibilities.append([
							-label0,
							-label1,
							-label2,
							-label3,
						])

	all_possibilities = []
	for tile in possibilities:
		for tr in all_transforms:
			all_possibilities.append((transform_tile(tile, tr), tr))

	votes = defaultdict(lambda: defaultdict(int))
	consolidated_votes = defaultdict(int)
	tile_map = {}
	for tile, tr in all_possibilities:
		k = hash_tile(tile)
		votes[k][tr] += 1
		consolidated_votes[k] += 1
		tile_map[k] = tile

	print(len(neighborhood_labels))
	print(len(all_possibilities))
	print(len(consolidated_votes))

	return votes, consolidated_votes, tile_map

def findBest(votes, consolidated_votes, tile_map):
	best = None
	best_count = None
	second_best = None
	second_best_count = None
	for k, count in consolidated_votes.items():
		if best_count is None or count > best_count:
			second_best_count = best_count
			second_best = best
			best_count = count
			best = k
		elif second_best_count is None or count > second_best_count:
			second_best_count = count
			second_best = k
	
	return (
		best,
		best_count,
		second_best,
		second_best_count,
	)

def findBest2(votes, consolidated_votes, tile_map):
	best = None
	best_score = None
	second_best = None
	second_best_score = None
	for k, count in consolidated_votes.items():
		score = Score()
		for label in tile_map[k]:
			if label < 0:
				score.flipped_edges += 1
		for tr, subcount in votes[k].items():
			score.total_votes += subcount
			if tr.isIdentity():
				score.untransformed_votes = count
		assert(score.total_votes == count)

		if best_score is None or score > best_score:
			second_best_score = best_score
			second_best = best
			best_score = score
			best = k
		elif second_best_score is None or score > second_best_score:
			second_best_score = score
			second_best = k
	
	return (
		best,
		best_score,
		second_best,
		second_best_score,
	)

def importVotes(raw_votes, tile_map):
	check_votes = defaultdict(lambda: defaultdict(int))
	check_consolidated_votes = defaultdict(int)
	for v in raw_votes:
		tile = v["tile"]
		k = hash_tile(tile)
		tile_map[k] = tile

		assert(check_consolidated_votes[k] == 0)

		for entry in v["counts"]:
			tr = entry["transform"]
			tr = Transform(
				tr["flipX"],
				tr["flipY"],
				tr["rotation"],
			)
			count = entry["count"]
			assert(check_votes[k][tr] == 0)
			check_votes[k][tr] += count
			check_consolidated_votes[k] += count
	return check_votes, check_consolidated_votes

def main():
	with open("C:/tmp/tile-suggestion.json") as f:
		data = json.load(f)

	neighborhood_labels = data['impossibleNeighborhoodLabels']
	check_votes = data['votes']
	check_best_tile = data['tile']
	check_second_best_tile = data['alternativeTile']

	(
		votes,
		consolidated_votes,
		tile_map,
	) = consolidateVotes(neighborhood_labels)
	(
		best,
		best_count,
		second_best,
		second_best_count,
	) = findBest2(votes, consolidated_votes, tile_map)

	print(f"best = {tile_map[best]} (count = {best_count})")
	#print(f"second_best = {tile_map[second_best]} (count = {second_best_count})")
	print(f"check_best = {check_best_tile}")
	#print(f"check_second_best = {check_second_best_tile}")

	check_votes, check_consolidated_votes = importVotes(data['votes'], tile_map)

	(
		recheck_best,
		recheck_best_count,
		recheck_second_best,
		recheck_second_best_count,
	) = findBest2(check_votes, check_consolidated_votes, tile_map)

	print(f"recheck_best = {tile_map[recheck_best]} (count = {recheck_best_count})")
	#print(f"recheck_second_best = {tile_map[recheck_second_best]} (count = {recheck_second_best_count})")

if __name__ == "__main__":
	main()
