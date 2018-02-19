# Load Testing Chatsript With Apache JMeter

## Requirements

Apache JMeter 3.1 or above

## Running the tests

Open up the WorkBench.jmx file from inside apache jmeter.  You should see a test plan populated with a basic tcp sampler run by a thread group.  Edit the TCP Sampler to reflect the host and port that your ChatScript server is running on.

The basic test in this file simply generates a single request using a unique username and queries the default bot harry.   This test plan could be a lot more complex, but for now it can give a rough baseline.

## Example results

### Mac results - 1000 concurrent users, 5 iterations
<table>
<tr>
<th># Samples</th><th>Average</th><th>Min</th><th>Max</th><th>Std. Dev.</th><th>Error %</th><th>Throughput</th>
</tr>
<tr>
<td>5000</td><td>2</td><td>1</td><td>32</td><td>2.32</td><td>0.000%</td><td>339.51246</td>
</tr>
</table>

### Linux results - 1000 concurrent users, 5 iterations
<table>
<tr>
<th># Samples</th><th>Average</th><th>Min</th><th>Max</th><th>Std. Dev.</th><th>Error %</th><th>Throughput</th>
</tr>
<tr>
<td>5000</td><td>0</td><td>0</td><td>29</td><td>0.84</td><td>0.000%</td><td>354.15781</td>
</tr>
</table>