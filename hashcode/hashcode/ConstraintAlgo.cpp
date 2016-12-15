#include "ConstraintAlgo.hpp"

#include <vector>
#include <unordered_set>

typedef GeoPhotographIndex::index<PhotoLat>::type Photos_by_lat;
typedef ShootMutliIndex::index<ColIndex>::type    Shoots_by_col;
typedef ShootMutliIndex::index<PhotoIndex>::type  Shoots_by_photo;

const double LOG_INTERVAL = .01; // 1%
const double MAX_FREQ = 2000; //TODO optimize

void ConstraintAlgo::buildPhotographIndex() {

	std::vector<Collection*>& collections = this->simulation->getCollections();

	for(auto c = collections.begin(); c != collections.end(); c++) {
		std::vector<Photograph*>& photographs = (*c)->getPhotographs();

		for (auto p = photographs.begin(); p != photographs.end(); p++) {
			this->photosIndex.insert(*p);
		}
	}
}

void ConstraintAlgo::generateShoots() { // TODO paralléliser ça

	Simulation* s = this->simulation;
	std::vector<Satellite*>& satellites = this->simulation->getSatellites();

	// for each turn, for each satellite, find photographs it can reach
	for (unsigned int t = 0; t < s->getDuration(); t++) {

		// log progression
		if (t % int(s->getDuration() * LOG_INTERVAL) == 0) {
			std::cout << t * 100 / s->getDuration() << "%" << std::endl;
		}

		for (auto sat = satellites.begin(); sat != satellites.end(); sat++) {

			Satellite* satellite = *sat;

			int d = satellite->getOrientationMaxValue();
			LocationUnit lat       = satellite->getLatitudeT(t);
			LocationUnit longitude = satellite->getLongitudeT(t);

			//TODO handle max latitude
			std::pair<LocationUnit, LocationUnit>
				lat_bounds(lat - d + 1, lat + d);
			//TODO handle border cases
			std::pair<LocationUnit, LocationUnit>
				long_bounds(longitude - d , longitude + d);

			Photos_by_lat& latitudeIndex = this->photosIndex.get<PhotoLat>();

			// photo we can reach in latitude
			Photos_by_lat::iterator
				lat_lower = latitudeIndex.lower_bound(lat_bounds.first),
				lat_upper = latitudeIndex.upper_bound(lat_bounds.second);

			for (auto it = lat_lower; it != lat_upper; it++) {
				Photograph* p = *it;
				if (long_bounds.first < p->getLongitude()
						&& long_bounds.second > p->getLongitude()) {
					for (Collection* col: p->getCollections()) {
						this->shoots.insert(Shoot(col, p, satellite, t));
					}
				}
			}
		}
	}
}

void ConstraintAlgo::cleanCollections() {

	std::vector<Collection*>& collections = this->simulation->getCollections();

	Shoots_by_col& shootsIndex = this->shoots.get<ColIndex>();

	for (Collection* col: collections) {

		std::pair<Shoots_by_col::iterator, Shoots_by_col::iterator> p =
			shootsIndex.equal_range(col);

		std::unordered_set<Photograph*> photos_in_col;
		for (auto it = p.first; it != p.second; it++) {
			photos_in_col.insert(it->m_photograph);
		}

		// missing photographs
		if (col->getNumberOfPhotographs() > photos_in_col.size()) {
			std::cout << "Remove " << *col << std::endl;
			shootsIndex.erase(p.first, p.second);
		}

	}

}

void ConstraintAlgo::initConstraints() {

	Shoots_by_photo& shootsIndex = this->shoots.get<PhotoIndex>();

	for (const Shoot s : shootsIndex) {
		this->constraints[s.m_photograph]++;
	}

	for (const std::pair<Photograph*, unsigned int>& p : this->constraints) {
		this->constraints[p.first] = p.first->getValue() + MAX_FREQ - p.second;
	}

}

void ConstraintAlgo::solve(Simulation* s) {

	this->simulation = s;

	std::cout << "hello world" << std::endl;

	std::cout << "Start building photographs index" << std::endl;
	this->buildPhotographIndex();
	std::cout << "End building photographs index" << std::endl;

	std::cout << "Start looking for photo we can shoot" << std::endl;
	this->generateShoots();
	std::cout << "End looking for photo we can shoot" << std::endl;

	std::cout << "Remove collections we can not complete" << std::endl;
	this->cleanCollections();
	std::cout << "End collections removal" << std::endl;

	std::cout << "Init constraints" << std::endl;
	this->initConstraints();
	std::cout << "End constraints init" << std::endl;

	for (auto p : this->constraints) {
		std::cout << *p.first << " " << p.second<< std::endl;
	}

	std::cout << "end" << std::endl;

}
