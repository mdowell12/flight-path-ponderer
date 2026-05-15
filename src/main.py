import json
from pprint import pprint
import time

from opensky_api import OpenSkyApi
import requests

BASE_URL = "https://api.adsb.lol"


def get_nearest_aircraft(lat, lon) -> dict:
    api = OpenSkyApi()
    margin = 0.02  # Roughly 1.5 miles
    states = api.get_states(bbox=_get_bbox_for_point(lat, lon, margin))
    nearest_aircraft = None
    all_aircraft = []
    min_distance = float('inf')

    # for state in states.states:
    #     # if state.latitude is not None and state.longitude is not None:
    #     distance_from_center = ((state.latitude - lat) ** 2 + (state.longitude - lon) ** 2) ** 0.5
    #     # if distance_from_center < min_distance:
    #     #     min_distance = distance_from_center
    #     aircraft = {
    #         "icao24": state.icao24,
    #         "callsign": state.callsign,
    #         "latitude": state.latitude,
    #         "longitude": state.longitude,
    #         "altitude": state.baro_altitude,
    #         "velocity": state.velocity,
    #         "distance": distance_from_center
    #     }
        
        # current_time = int(time.time())
        # flights = api.get_flights_by_aircraft(state.icao24, states.time - 3600 * 24 * 2, states.time + 3600 * 24)
        # import pdb; pdb.set_trace()
        # flights = api.get_flights_by_aircraft(state.icao24, current_time, current_time+1)
        
        # all_aircraft.append(aircraft)
        # if flights:
        #     aircraft["flights"] = [{
        #         "firstSeen": flight.firstSeen,
        #         "lastSeen": flight.lastSeen,
        #         "estDepartureAirport": flight.estDepartureAirport,
        #         "estArrivalAirport": flight.estArrivalAirport
        #     } for flight in flights]
    closest_plane = _get_closest_plane(lat, lon, radius=2)

    if closest_plane:
        route_info = _get_route_info([closest_plane["flight"].strip()], lat, lon)
    else:
        route_info = []

    closest_plane["route_info"] = route_info[0] if route_info else {}

    # Help ourselves out by adding departure and arrival airport info
    closest_plane["departure_airport"] = {"location": "Unknown"}
    closest_plane["arrival_airport"] = {"location": "Unknown"}

    if airports := closest_plane["route_info"].get("_airports"):
        if len(airports) > 1:
            # TODO Handle multi-leg flights, e.g. KSEA-KPHX-KSEA for flight DAL2449
            closest_plane["departure_airport"] = airports[-2]
            closest_plane["arrival_airport"] = airports[-1]

    import pdb; pdb.set_trace()
    
    return closest_plane


def _get_closest_plane(lat, lon, radius) -> dict:
    """
    Get the closest plane to the given latitude and longitude within the specified radius.    
    
    :param lat: Description
    :param lon: Description
    :param radius: Description
    :return: Description
    :rtype: dict
    """
    url = f"{BASE_URL}/v2/closest/{lat}/{lon}/{radius}"
    
    result = requests.get(url, headers={"Accept": "application/json"})
    result.raise_for_status()
    data = result.json()
    if len(data["ac"]) == 1:
        return data["ac"][0]
    else:
        return {}


def _get_route_info(callsigns: set[str], lat, lon) -> dict:
    url = f"{BASE_URL}/api/0/routeset"
    body = {
        "planes": [
            {
                "callsign": c.strip(),
                "lat": lat,
                "lng": lon,
            } for c in callsigns
        ]
    }
    result = requests.post(url, json=body)
    result.raise_for_status()
    return result.json()


def _get_bbox_for_point(lat, lon, margin) -> tuple:
    return (lat - margin, lat + margin, lon - margin, lon + margin)


def lambda_handler(event, context):
    mock_flight_data = {
        "flight": "DAL2449",
        "origin": "KSEA",
        "origin_city": "Seattle",
        "destination": "KPHX",
        "destination_city": "Phoenix",
        "aircraft_type": "B739",
        "altitude_ft": 35000,
        "speed_kts": 450,
        "eta_minutes": 87,
    }

    return {
        "statusCode": 200,
        "body": json.dumps(mock_flight_data)
    }


if __name__ == "__main__":
    while True:
        result = get_nearest_aircraft(47.61718184635572, -122.31581513150061)  # Coordinates for Seattle, USA
        # import pdb; pdb.set_trace()
        message = f"""
=================
Nearest Aircraft:
    Flight {result.get("flight", "").strip()} is headed from {result.get("departure_airport", {}).get("location")} to {result.get("arrival_airport", {}).get("location")}
    Altitude: {result.get("alt_baro")} ft. descending at a rate of {result.get("baro_rate")} ft/min

"""
        print(message)
        time.sleep(30)