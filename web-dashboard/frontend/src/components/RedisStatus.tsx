import React, { useState, useEffect ,useRef} from 'react';
import { api, Task, QueueStats } from '../services/api';

const RedisStatus: React.FC = () => {
  const [tasks, setTasks] = useState<Task[]>([]);
  const [queueStats, setQueueStats] = useState<QueueStats | null>(null);
  const [searchTaskId, setSearchTaskId] = useState('');
  const [searchResult, setSearchResult] = useState<Task | null>(null);
  const [loading, setLoading] = useState(false);
  const isFetching = useRef(false);

const fetchData = async () => {
    if (isFetching.current) return; // prevent overlap
    isFetching.current = true;
  try {
    const [tasksData, statsData] = await Promise.all([
      api.getTasks(),
      new Promise(res => setTimeout(res, 200)).then(() => api.getQueueStats())
    ]);
    setTasks([...tasksData]); 
    setQueueStats({ ...statsData });
  } catch (err) {
    console.error('Error fetching data:', err);
  }finally {
    isFetching.current = false; // ✅ ensures next refresh always allowed
  }
};
const handleRefresh = async () => {
  setLoading(true);
  await fetchData();
  setTimeout(async () => {
    await fetchData();
    setLoading(false);
  }, 1000);
};


useEffect(() => {
  fetchData();
  const interval = setInterval(fetchData, 2000); // every 5s
  return () => clearInterval(interval);
}, []);


  const handleSearch = async () => {
    if (!searchTaskId.trim()) return;

    setLoading(true);
    try {
      const task = await api.getTask(searchTaskId);
      setSearchResult(task);
    } catch (err) {
      setSearchResult(null);
      alert('Task not found!');
    } finally {
      setLoading(false);
    }
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'completed': return 'bg-gray-700';
      case 'processing': return 'bg-gray-700';
      case 'pending': return 'bg-gray-700';
      default: return 'bg-gray-700';
    }
  };

  const getStatusIcon = (status: string) => {
    switch (status) {
      case 'completed': return '✓';
      case 'processing': return '⚙';
      case 'pending': return '⏳';
      default: return '?';
    }
  };

  const getPriorityColor = (priority: string) => {
    switch (priority) {
      case 'critical': return 'bg-gray-700';
      case 'high': return 'bg-gray-700';
      case 'normal': return 'bg-gray-700';
      case 'low': return 'bg-gray-700';
      default: return 'bg-gray-700';
    }
  };

  return (
    <div className="bg-gray-800 rounded-lg p-6 shadow-lg h-full overflow-hidden flex flex-col">
      <h2 className="text-2xl font-bold mb-4 text-white">
        Redis Server Status
      </h2>

      {/* Queue Statistics */}
{/* {queueStats && (
  <div className="mb-6 grid grid-cols-5 gap-4">
    {[
      { label: "Critical", value: queueStats.critical },
      { label: "High", value: queueStats.high },
      { label: "Normal", value: queueStats.normal },
      { label: "Low", value: queueStats.low },
      { label: "Done", value: queueStats.completed },
    ].map((item) => (
      <div
        key={item.label}
        className="rounded-xl p-4 text-center shadow-lg bg-gray-700 hover:scale-[1.03] transition-transform duration-200"
      >
        <div className="text-white text-sm font-semibold mb-1 tracking-wide uppercase opacity-90">
          {item.label}
        </div>
      </div>
    ))}
  </div>
)} */}


      {/* Search Box */}
      <div className="mb-4">
        <div className="flex gap-2">
          <input
            type="text"
            value={searchTaskId}
            onChange={(e) => setSearchTaskId(e.target.value)}
            onKeyPress={(e) => e.key === 'Enter' && handleSearch()}
            placeholder="Search task by ID (e.g., task:1000)"
            className="flex-1 px-4 py-2 bg-gray-700 text-white rounded-lg border border-gray-600 focus:outline-none focus:ring-2 focus:ring-blue-500"
          />
          <button
            onClick={handleSearch}
            disabled={loading}
            className="px-6 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 disabled:bg-gray-600"
          >
            Search
          </button>
        </div>

        {/* Search Result */}
        {searchResult && (
          <div className="mt-3 p-4 bg-gray-900 rounded-lg border border-gray-700">
            <h3 className="text-sm font-semibold text-gray-400 mb-2">Search Result</h3>
            <div className="text-sm text-gray-300 space-y-1">
              <div><span className="text-gray-500">ID:</span> <span className="font-mono">{searchResult.id}</span></div>
              <div><span className="text-gray-500">Type:</span> {searchResult.type}</div>
              <div><span className="text-gray-500">Priority:</span> <span className={`px-2 py-1 rounded text-xs ${getPriorityColor(searchResult.priority)}`}>{searchResult.priority}</span></div>
              <div><span className="text-gray-500">Status:</span> <span className={getStatusColor(searchResult.status)}>{getStatusIcon(searchResult.status)} {searchResult.status}</span></div>
            </div>
          </div>
        )}
      </div>

      {/* Refresh Button */}
      <button
       onClick={handleRefresh}
      disabled={loading}
        className="mb-4 w-full py-2 bg-gray-700 text-white rounded-lg hover:bg-gray-600 transition-colors"
      >
        Refresh
      </button>

      {/* Tasks List - Last 5 only */}
      <div className="flex-1 overflow-y-auto">
        <h3 className="text-lg font-semibold text-gray-300 mb-3">Recent Tasks (Last 5)</h3>
        <div className="space-y-2">
          {tasks.length === 0 ? (
            <div className="text-center text-gray-500 py-8">
              No tasks yet. Create one to get started!
            </div>
          ) : (
            tasks.slice(0, 5).map((task) => (
              <div
                key={task.id}
                className="bg-gray-900 rounded-lg p-3 border border-gray-700 hover:border-gray-600 transition-colors"
              >
                <div className="flex items-center justify-between">
                  <div className="flex-1">
                    <div className="flex items-center gap-2">
                      <span className="text-xs font-mono text-gray-500">{task.id}</span>
                      <span className={`px-2 py-0.5 rounded text-xs ${getPriorityColor(task.priority)}`}>
                        {task.priority}
                      </span>
                    </div>
                    <div className="text-sm text-white mt-1">{task.type}</div>
                  </div>
                  <div className={`text-sm font-semibold ${getStatusColor(task.status)}`}>
                    {getStatusIcon(task.status)} {task.status}
                  </div>
                </div>
              </div>
            ))
          )}
        </div>
      </div>
    </div>
  );
};

export default RedisStatus;
