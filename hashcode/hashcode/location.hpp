#pragma once

/**
*	Location class
**/
class location{

	private:
		double m_latitude;
		double m_longitude;
	public:
		location(double, double);
		~location();
		location(const location& location);
		location& operator=(const location& location);

};